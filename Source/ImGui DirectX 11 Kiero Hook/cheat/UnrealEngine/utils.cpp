#include "utils.h"
#include <Psapi.h> // used for MODULEINFO and some other stuff
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <mutex>
#include <unordered_map>

// ============ 崩溃诊断日志 + VEH 崩溃捕获 ============
namespace dbg
{
	// 日志路径：%TEMP%\sws_trainer.log（C:\ 根目录普通权限写不进去，这是上一版日志缺失的原因）
	static char g_logPath[MAX_PATH] = {};

	// 用 Win32 API 直写一行（异常处理器内安全：不碰 CRT 堆锁）
	static void RawLog(const char* str)
	{
		if (!g_logPath[0]) return;

		HANDLE h = CreateFileA(g_logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h == INVALID_HANDLE_VALUE) return;

		DWORD written = 0;
		WriteFile(h, str, (DWORD)strlen(str), &written, nullptr);
		CloseHandle(h);
	}

	void Log(const char* fmt, ...)
	{
		if (!g_logPath[0]) return;

		FILE* f = nullptr;
		if (fopen_s(&f, g_logPath, "a") != 0 || !f)
			return;

		SYSTEMTIME st;
		GetLocalTime(&st);
		fprintf_s(f, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		va_list args;
		va_start(args, fmt);
		vfprintf_s(f, fmt, args);
		va_end(args);

		fprintf_s(f, "\n");
		fclose(f);
	}

	void SetStep(const char* name)
	{
		if (!name || LastStep == name)
			return;

		LastStep = name;

		// 节流：200ms 内最多记一条步骤日志（每帧步骤会循环变化，全记会产生海量日志）
		static DWORD lastLogTick = 0;
		const DWORD now = GetTickCount();
		if (now - lastLogTick >= 200)
		{
			lastLogTick = now;
			Log("step: %s", name);
		}
	}

	// 只记录致命异常（访问违例/非法指令等）；C++ 异常与断点属于正常控制流，直接放行
	static bool IsFatalException(const DWORD code)
	{
		return code == EXCEPTION_ACCESS_VIOLATION ||		// 0xC0000005 读写非法内存
			code == EXCEPTION_ILLEGAL_INSTRUCTION ||		// 0xC000001D
			code == EXCEPTION_IN_PAGE_ERROR ||			// 0xC0000006
			code == EXCEPTION_STACK_OVERFLOW ||			// 0xC00000FD
			code == 0xC0000409;							// 栈缓冲区溢出 / fast fail
	}

	static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep)
	{
		// 递归/重复进入保护（仅记录第一次致命异常）
		static volatile LONG handled = 0;
		if (InterlockedCompareExchange(&handled, 1, 0) != 0)
			return EXCEPTION_CONTINUE_SEARCH;

		const EXCEPTION_RECORD* er = ep->ExceptionRecord;
		if (!IsFatalException(er->ExceptionCode))
		{
			// 非致命异常放行且不占用标志位
			InterlockedExchange(&handled, 0);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		const uintptr_t addr = (uintptr_t)er->ExceptionAddress;

		// 计算崩溃地址归属（模块名 + 模块内偏移）
		char where[128];
		if (ModuleBase && addr >= ModuleBase && addr < ModuleBaseEnd)
		{
			sprintf_s(where, "GameExe+%llX", (unsigned long long)(addr - ModuleBase));
		}
		else
		{
			HMODULE hMod = nullptr;
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				(LPCSTR)addr, &hMod))
			{
				char name[MAX_PATH] = "?";
				GetModuleFileNameA(hMod, name, MAX_PATH);
				const char* base = strrchr(name, '\\');
				base = base ? base + 1 : name;
				sprintf_s(where, "%s+%llX", base, (unsigned long long)(addr - (uintptr_t)hMod));
			}
			else
			{
				sprintf_s(where, "unknown");
			}
		}

		const char* access = "";
		if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
			access = er->ExceptionInformation[0] == 0 ? " READ" : (er->ExceptionInformation[0] == 1 ? " WRITE" : " EXEC");

		const CONTEXT* ctx = ep->ContextRecord;
		char buf[640];
		sprintf_s(buf,
			"=== CRASH 0x%08X%s at %s ===\r\n"
			"LastStep: %s\r\n"
			"FaultingAddr: 0x%llX  Param0=%llu\r\n"
			"RIP=%llX RSP=%llX RCX=%llX RDX=%llX R8=%llX R9=%llX RAX=%llX\r\n",
			er->ExceptionCode, access, where, LastStep,
			(unsigned long long)er->ExceptionInformation[1], (unsigned long long)er->ExceptionInformation[0],
			(unsigned long long)ctx->Rip, (unsigned long long)ctx->Rsp,
			(unsigned long long)ctx->Rcx, (unsigned long long)ctx->Rdx,
			(unsigned long long)ctx->R8, (unsigned long long)ctx->R9,
			(unsigned long long)ctx->Rax);
		RawLog(buf);

		// 放行给 UE4 的崩溃处理（弹窗照常出现）
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void InstallCrashHandler()
	{
		GetTempPathA(MAX_PATH, g_logPath);
		strcat_s(g_logPath, "sws_trainer.log");

		// 第一优先级 VEH：先于 UE4 自己的 SEH 崩溃处理拿到异常
		AddVectoredExceptionHandler(1, CrashHandler);

		Log("=== Session start (log: %%TEMP%%\\sws_trainer.log, VEH installed) ===");
	}
}

namespace mem
{
	__forceinline uintptr_t staticAddress(uintptr_t base, const std::vector<uintptr_t>& offsets)
	{
		for (const uintptr_t& offset : offsets)
			base = *(uintptr_t*)base + offset;
		return base;
	}

	template <typename T>
	__forceinline T readSafe(uintptr_t address)
	{
		T buffer;
		static const HANDLE hProcess = GetCurrentProcess();
		ReadProcessMemory(hProcess, (void*)address, &buffer, sizeof(T), nullptr);
		return buffer;
	}

	// sometimes some pointers will be pointing to invalid memory and by using this function we can prevent crashes
	__forceinline uintptr_t GetPointerSafe(uintptr_t base, const std::vector<uintptr_t>& offsets)
	{
		for (const uintptr_t& offset : offsets)
		{
			base = readSafe<uintptr_t>(base);
			if (!base) return 0;
			base += offset;
		}
		return base;
	}

	// ============ 名字解析：调用游戏 FName::AppendString（Dumper-7 同款方案） ============
	// 不再依赖 GNames 偏移与其内部布局 —— 4.22.3 的 TNameEntryArray 布局无法可靠验证，
	// 直接调用游戏自身函数（RVA 0x5FFCA0）解析，结果全量缓存（名字表静态只增）

	// AppendString 签名: void FName::AppendString(const FName* this, FString& Out)
	using AppendString_t = void(*)(const void*, void*);
	static AppendString_t AppendStringFn = nullptr;

	// SEH 保护的 AppendString 调用（纯 POD 局部，无 C++ 对象展开，可与 __try 共存）
	static bool AppendStringSafe(const int FNameIndex, wchar_t* const wbuf, const int wbufMax)
	{
		struct FNamePOD { int32_t ComparisonIndex; int32_t Number; } fname{ FNameIndex, 0 };
		struct FStringPOD { wchar_t* Data; int32_t Num; int32_t Max; } fstr{ wbuf, 0, wbufMax };

		__try
		{
			AppendStringFn(&fname, &fstr);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool InitNameResolver()
	{
		if (AppendStringFn)
			return true;

		AppendStringFn = (AppendString_t)(ModuleBase + APPENDSTRING_RVA);

		// 用 FName 索引 0 验证（恒为 "None"）
		wchar_t wbuf[2048] = {};
		if (!AppendStringSafe(0, wbuf, 2047) ||
			wbuf[0] != L'N' || wbuf[1] != L'o' || wbuf[2] != L'n' || wbuf[3] != L'e' || wbuf[4] != L'\0')
		{
			AppendStringFn = nullptr;
			return false;
		}

		return true;
	}

	const char* GetGName(const int FNameIndex)
	{
		static std::mutex cacheLock;
		static std::unordered_map<int, std::string> cache;
		static const std::string empty;

		if (FNameIndex < 0 || !AppendStringFn)
			return empty.c_str();

		{
			std::lock_guard<std::mutex> guard(cacheLock);
			const auto it = cache.find(FNameIndex);
			if (it != cache.end())
				return it->second.c_str();
		}

		// 未缓存：调游戏函数解析（游戏名字最长 1024 宽字符，2048 缓冲保证不触发 FString 重分配）
		wchar_t wbuf[2048] = {};
		if (!AppendStringSafe(FNameIndex, wbuf, 2047))
			return empty.c_str();

		char nbuf[256];
		int j = 0;
		for (; wbuf[j] && j < 255; j++)
			nbuf[j] = (char)wbuf[j];
		nbuf[j] = '\0';

		std::lock_guard<std::mutex> guard(cacheLock);
		const auto [ins, inserted] = cache.emplace(FNameIndex, nbuf);
		return ins->second.c_str();
	}

	namespace
	{
		// ============ GObjects（TUObjectArray，Dumper-7 Basic.hpp 布局） ============
		// TUObjectArray: Objects(FUObjectItem** @0x00) MaxElements(@0x10) NumElements(@0x14) MaxChunks(@0x18) NumChunks(@0x1C)
		// FUObjectItem: Object(@0x00) + 0x10 pad，总大小 0x18
		uintptr_t GObjectsChunks = 0; // chunk 指针数组
		int32_t GObjectsNumElements = 0;
		int32_t GObjectsNumChunks = 0;
		bool bGObjectsInit = false;

		// 按索引取 UObject（与引擎内部遍历方式相同：chunk 数组已验证，直接读）
		UObject* GetObjectByIndex(const int index)
		{
			if (index < 0 || index >= GObjectsNumElements)
				return nullptr;

			const int chunkIdx = index / GOBJECTS_CHUNK;
			if (chunkIdx >= GObjectsNumChunks)
				return nullptr;

			const uintptr_t chunk = *(uintptr_t*)(GObjectsChunks + (uintptr_t)chunkIdx * sizeof(void*));
			if (!chunk)
				return nullptr;

			return *(UObject**)(chunk + (uintptr_t)(index % GOBJECTS_CHUNK) * 0x18);
		}
	}

	bool InitGObjects()
	{
		if (bGObjectsInit)
			return true;

		static const HANDLE hProcess = GetCurrentProcess();

		// 读 TUObjectArray 头（静态 RVA 来自 Dumper-7）
		struct { uintptr_t Objects; uintptr_t pad; int32_t MaxElements, NumElements, MaxChunks, NumChunks; } arr{};
		if (!ReadProcessMemory(hProcess, (void*)(ModuleBase + GOBJECTS_RVA), &arr, sizeof(arr), nullptr))
			return false;

		// 基本有效性验证
		if (!arr.Objects || arr.NumElements <= 0 || arr.NumElements > 0x10000000 || arr.NumChunks <= 0 || arr.NumChunks > 0x1000)
			return false;

		// 验证第一个 chunk 可读且首个对象指针有效
		uintptr_t chunk0 = 0;
		if (!ReadProcessMemory(hProcess, (void*)arr.Objects, &chunk0, sizeof(chunk0), nullptr) || !chunk0)
			return false;
		uintptr_t firstObj = 0;
		if (!ReadProcessMemory(hProcess, (void*)chunk0, &firstObj, sizeof(firstObj), nullptr))
			return false;
		if (!firstObj)
			return false;

		GObjectsChunks = arr.Objects;
		GObjectsNumElements = arr.NumElements;
		GObjectsNumChunks = arr.NumChunks;
		bGObjectsInit = true;
		return true;
	}

	UObject* FindObjectByName(const char* name)
	{
		if (!bGObjectsInit && !InitGObjects())
			return nullptr;

		const char first = name[0];
		for (int i = 0; i < GObjectsNumElements; i++)
		{
			UObject* obj = GetObjectByIndex(i);
			if (!obj)
				continue;

			const int nameIdx = *(int*)((uintptr_t)obj + OBJ_FNAME);
			const char* n = GetGName(nameIdx);
			if (!n || n[0] != first) // 先比首字符再 strcmp，加速遍历
				continue;

			if (strcmp(n, name) == 0)
				return obj;
		}

		return nullptr;
	}

	UObject* FindClassFunction(UObject* Class, const char* funcName)
	{
		if (!Class)
			return nullptr;

		// 沿 SuperStruct 链向上查找（自身 -> 父类 -> ...），防止自引用循环
		for (uintptr_t s = (uintptr_t)Class; s; s = *(uintptr_t*)(s + STRUCT_SUPER))
		{
			for (uintptr_t child = *(uintptr_t*)(s + STRUCT_CHILDREN); child; child = *(uintptr_t*)(child + FIELD_NEXT))
			{
				const char* n = GetGName(*(int*)(child + OBJ_FNAME));
				if (n && strcmp(n, funcName) == 0)
					return (UObject*)child;
			}
		}

		return nullptr;
	}

	namespace
	{
		// 反射调用缓存
		void* TraceLineFunc = nullptr;      // LineTraceSingle 的 UFunction*
		void* KismetSystemLibCDO = nullptr; // UKismetSystemLibrary 的 ClassDefaultObject
		void* ProcessEventFn = nullptr;     // UObject::ProcessEvent（从 CDO vtable 取）
	}

	// UObject::ProcessEvent 虚函数索引（Dumper-7: Offsets::ProcessEventIdx = 0x40，vtable 索引）
	constexpr int PROCESSEVENT_VT_INDEX = 0x40;

	bool InitTraceLine()
	{
		if (TraceLineFunc)
			return true;

		if (!InitGObjects())
		{
			dbg::Log("[TL] InitGObjects failed (RVA 0x%X)", GOBJECTS_RVA);
			return false;
		}

		if (!InitNameResolver())
		{
			dbg::Log("[TL] InitNameResolver failed (AppendString RVA 0x%X)", APPENDSTRING_RVA);
			return false;
		}

		// 找 UKismetSystemLibrary 类（类对象的短名称即类名）
		UObject* kismetClass = FindObjectByName("KismetSystemLibrary");
		if (!kismetClass)
		{
			dbg::Log("[TL] FindObjectByName(KismetSystemLibrary) failed, %d objects scanned", GObjectsNumElements);
			return false;
		}

		// 类的 CDO 作为 static 函数调用的 this
		KismetSystemLibCDO = *(void**)((uintptr_t)kismetClass + CLASS_CDO);
		if (!KismetSystemLibCDO)
		{
			dbg::Log("[TL] KismetSystemLibrary CDO null");
			return false;
		}

		// 找 LineTraceSingle UFunction（含父类链）
		TraceLineFunc = FindClassFunction(kismetClass, "LineTraceSingle");
		if (!TraceLineFunc)
		{
			dbg::Log("[TL] FindClassFunction(LineTraceSingle) failed");
			return false;
		}
		dbg::Log("[TL] reflection OK: class=0x%llX CDO=0x%llX func=0x%llX",
			(unsigned long long)kismetClass, (unsigned long long)KismetSystemLibCDO, (unsigned long long)TraceLineFunc);

		// ProcessEvent 优先用 Dumper-7 静态 RVA（权威值），并校验函数开头字节
		// （UE4.16 ProcessEvent 序言: 40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC）
		{
			static const HANDLE hProcess = GetCurrentProcess();
			byte prologue[6] = {};
			if (ReadProcessMemory(hProcess, (void*)(ModuleBase + PROCESSEVENT_RVA), prologue, sizeof(prologue), nullptr) &&
				prologue[0] == 0x40 && prologue[1] == 0x55 && prologue[2] == 0x56 &&
				prologue[3] == 0x57 && prologue[4] == 0x41 && prologue[5] == 0x54)
			{
				ProcessEventFn = (void*)(ModuleBase + PROCESSEVENT_RVA);
				return true;
			}
		}

		// 兜底：从 vtable 取 VTable[0x40]（索引语义）
		const uintptr_t vtable = *(uintptr_t*)KismetSystemLibCDO;
		if (!vtable)
			return false;
		ProcessEventFn = *(void**)(vtable + (uintptr_t)PROCESSEVENT_VT_INDEX * sizeof(void*));
		if (!ProcessEventFn)
			return false;

		return true;
	}

	bool CallLineTraceSingle(void* WorldContextObject, const vec3& start, const vec3& end,
		void* const* actorsToIgnore, const int actorCount, const int traceChannel, FHitResult& outHit)
	{
		if (!TraceLineFunc && !InitTraceLine()) // lazy 重试：注入过早时可自动恢复
			return false;

		// UKismetSystemLibrary::LineTraceSingle 参数结构（Dumper-7 Engine_parameters.hpp，总大小 0xF0）
		struct FTraceParams
		{
			void* WorldContextObject;                                    // 0x00
			vec3 Start;                                                  // 0x08
			vec3 End;                                                    // 0x14
			byte TraceChannel;                                           // 0x20
			byte bTraceComplex;                                          // 0x21
			byte Pad1[0x6];                                              // 0x22
			struct { void* items; int count; int max; } ActorsToIgnore;  // 0x28
			byte DrawDebugType;                                          // 0x38
			byte Pad2[0x3];                                              // 0x39
			FHitResult OutHit;                                           // 0x3C
			byte bIgnoreSelf;                                            // 0xC4
			byte Pad3[0x3];                                              // 0xC5
			float TraceColor[4];                                         // 0xC8
			float TraceHitColor[4];                                      // 0xD8
			float DrawTime;                                              // 0xE8
			byte ReturnValue;                                            // 0xEC
			byte Pad4[0x3];                                              // 0xED
		};
		static_assert(sizeof(FTraceParams) == 0xF0, "LineTraceSingle 参数布局与最新版本不匹配");

		FTraceParams p{};
		p.WorldContextObject = WorldContextObject;
		p.Start = start;
		p.End = end;
		p.TraceChannel = (byte)traceChannel;
		p.bTraceComplex = 1;
		p.ActorsToIgnore.items = (void*)actorsToIgnore;
		p.ActorsToIgnore.count = actorCount;
		p.ActorsToIgnore.max = actorCount;
		p.bIgnoreSelf = 1;

		typedef void(*ProcessEvent_t)(const void*, void*, void*);
		((ProcessEvent_t)ProcessEventFn)(KismetSystemLibCDO, TraceLineFunc, &p);

		outHit = p.OutHit;
		return p.ReturnValue != 0;
	}

	bool CallUFunction(void* obj, void* func, void* params)
	{
		if (!obj || !func)
			return false;

		if (!ProcessEventFn && !InitTraceLine()) // 确保 ProcessEvent 已绑定
			return false;

		typedef void(*ProcessEvent_t)(const void*, void*, void*);
		((ProcessEvent_t)ProcessEventFn)(obj, func, params);
		return true;
	}

	uintptr_t GetModuleSize(const char* szModule)
	{
		if (HMODULE hModule = GetModuleHandleA(szModule))
		{
			MODULEINFO mInfo = { 0 };
			K32GetModuleInformation(GetCurrentProcess(), hModule, &mInfo, sizeof(MODULEINFO));
			return (uintptr_t)mInfo.SizeOfImage;
		}

		return 0;
	}

	uintptr_t PatternScan(const bool usedIdaStyleMask, std::string idaSignature)
	{
		auto idaSigToBytesArray = [&idaSignature, &usedIdaStyleMask](byte*& out_pattern, char*& out_mask, USHORT& out_len)->void
		{
			idaSignature.erase(remove(idaSignature.begin(), idaSignature.end(), ' '), idaSignature.end());

			out_len = idaSignature.length();
			out_pattern = new byte[out_len];
			out_mask = new char[out_len];

			USHORT realLen = 0;
			for (USHORT i = 0; i < out_len - 1; i++)
			{
				if (idaSignature[i] == '?')
				{
					out_pattern[realLen] = 0;
					out_mask[realLen] = '?';
					if (usedIdaStyleMask)
						i++;
					realLen++;
					continue;
				}

				char buf[2] = { idaSignature[i], idaSignature[i + 1] };
				sscanf_s(buf, "%02hhx", &out_pattern[realLen]);
				out_mask[realLen] = 'x';
				realLen++;
				i++;
			}

			out_len = realLen;
		};

		byte* pattern;
		char* mask;
		USHORT len;
		idaSigToBytesArray(pattern, mask, len);

		MEMORY_BASIC_INFORMATION mbi{};

		for (char* curr = (char*)ModuleBase; curr < (char*)(ModuleBaseEnd - len); curr += mbi.RegionSize)
		{
			if (!VirtualQuery(curr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) ||
				mbi.State != MEM_COMMIT ||
				mbi.Protect == PAGE_NOACCESS)
				continue;

			for (DWORD i = 0; i < mbi.RegionSize; i++)
			{
				bool bFound = true;
				for (DWORD j = 0; j < len; j++)
				{
					if (mask[j] != '?' && pattern[j] != *(byte*)(curr + i + j))
					{
						bFound = false;
						break;
					}
				}

				if (bFound)
				{
					delete[] pattern;
					delete[] mask;
					return (uintptr_t)(curr + i);
				}
			}
		}

		delete[] pattern;
		delete[] mask;
		return 0;
	}

	DWORD GetStaticOffset(byte offsetToRead, byte instructionLenTotal, const bool usedIdaStyleMask, const char* pattern)
	{
		uintptr_t pOffset = PatternScan(usedIdaStyleMask, pattern);
		if (!pOffset) return 0;

		DWORD oldProtect;
		if (VirtualProtect((void*)pOffset, sizeof(DWORD), PAGE_EXECUTE_READ, &oldProtect))
		{
			DWORD ret = (*(DWORD*)(pOffset + offsetToRead) + instructionLenTotal) + (pOffset - ModuleBase); // the first thing in brackets is the relative offset from the instruction, the second thing in brackets gives me the instruction offset, we need to add those numbers together to get the actual offset
			VirtualProtect((void*)pOffset, sizeof(DWORD), oldProtect, &oldProtect);
			return ret;
		}
		return 0;
	}
}

__forceinline float SmoothOutYaw(float targetYaw, float currentYaw, float smoothness)
{
	if (targetYaw > 0.f && currentYaw < 0.f)
	{
		float dist = 180.f - targetYaw + 180.f + currentYaw;
		if (dist < 180.f)
			return currentYaw - dist / smoothness;
	}
	else if (currentYaw > 0.f && targetYaw < 0.f)
	{
		float dist = 180.f - currentYaw + 180.f + targetYaw;
		if (dist < 180.f)
			return currentYaw + dist / smoothness;
	}

	return currentYaw + (targetYaw - currentYaw) / smoothness;
}

vec2 CalcAngle(const vec3& src, const vec3& dst, const vec2 currentAngles, float smoothness)
{
	vec3 delta = dst - src;

	float dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
	float pitch = (-((acosf((delta.z / dist)) * 180.0f / M_PI) - 90.0f));
	float yaw = atan2f(delta.y, delta.x) * (180.0f / M_PI);

	vec2 ret;
	ret.x = currentAngles.x + (pitch - currentAngles.x) / smoothness;
	ret.y = SmoothOutYaw(yaw, currentAngles.y, smoothness);
	ret.Normalize(); // makes the angles work for Sword With Sauce's angle system

	return ret;
}

vec3 GetVectorForward(const vec3& angles)
{
	float sp, sy, cp, cy;
	float angle;

	angle = angles.y * (M_PI / 180.0f);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = -angles.x * (M_PI / 180.0f);
	sp = sinf(angle);
	cp = cosf(angle);

	return { cp * cy, cp * sy, -sp };
}
