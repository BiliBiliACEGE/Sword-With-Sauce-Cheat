#pragma once
#include "UnrealClasses.h"
#include <array>
#include <vector>


namespace mem
{
	template<typename T, typename U> constexpr size_t OffsetOf(U T::* member)
	{
		return (char*)&((T*)nullptr->*member) - (char*)nullptr;
	}

	extern __forceinline uintptr_t staticAddress(uintptr_t base, const std::vector<uintptr_t>& offsets);

	extern __forceinline uintptr_t GetPointerSafe(uintptr_t base, const std::vector<uintptr_t>& offsets);

	uintptr_t GetModuleSize(const char* szModule);

	uintptr_t PatternScan(const bool usedIdaStyleMask, std::string idaSignature);

	DWORD GetStaticOffset(byte offsetToRead, byte extra, const bool usedIdaStyleMask, const char* pattern);

	// ============ UE 反射工具（版本无关，GObjects + 名称查找 + ProcessEvent 调用） ============

	// Dumper-7 导出的 GObjects RVA（TUObjectArray）。游戏大版本更新时需同步 Dumper-7 的 Offsets::GObjects
	constexpr DWORD GOBJECTS_RVA = 0x02F499F8;

	// FName::AppendString 静态 RVA（0x5FFCA0，profile 工具与 Dumper-7 双重确认）。
	// 名字解析直接调用游戏自身函数，无需 GNames 偏移与其内部布局（Dumper-7 同款方案）
	constexpr DWORD APPENDSTRING_RVA = 0x005FFCA0;

	// Dumper-7 导出的 UObject::ProcessEvent 静态 RVA（0x7896B0）
	constexpr DWORD PROCESSEVENT_RVA = 0x007896B0;

	// 初始化名字解析器：绑定 AppendString 并用 FName 索引 0（"None"）验证，成功返回 true
	bool InitNameResolver();

	// 解析 FName 索引为名字字符串（内部调用游戏 AppendString + 全量缓存，命中 O(1)）
	// 失败/无效索引返回空串。返回指针长期稳定（缓存节点不移动）
	const char* GetGName(const int FNameIndex);

	// UObject/UField/UStruct/UClass 字段偏移（Dumper-7 CoreUObject_classes.hpp）
	constexpr USHORT OBJ_INDEX = 0x0C;        // UObject::InternalIndex
	constexpr USHORT OBJ_CLASS = 0x10;        // UObject::ClassPrivate
	constexpr USHORT OBJ_FNAME = 0x18;        // UObject::NamePrivate（FName.ComparisonIndex）
	constexpr USHORT OBJ_OUTER = 0x20;        // UObject::OuterPrivate
	constexpr USHORT FIELD_NEXT = 0x28;       // UField::Next
	constexpr USHORT STRUCT_SUPER = 0x40;     // UStruct::SuperStruct
	constexpr USHORT STRUCT_CHILDREN = 0x48;  // UStruct::Children
	constexpr USHORT CLASS_CDO = 0xF8;        // UClass::ClassDefaultObject
	constexpr int GOBJECTS_CHUNK = 0x10000;   // TUObjectArray::ElementsPerChunk

	// 初始化 GObjects（静态 RVA + 有效性验证），成功返回 true
	bool InitGObjects();

	// 遍历 GObjects 按对象短名称查找（如 "KismetSystemLibrary"），失败返回 nullptr
	UObject* FindObjectByName(const char* name);

	// 在类（含父类链）的 Children 链表中查找指定名称的 UFunction，失败返回 nullptr
	UObject* FindClassFunction(UObject* Class, const char* funcName);

	// 初始化 UKismetSystemLibrary::LineTraceSingle 反射调用（CDO + UFunction + ProcessEvent）
	bool InitTraceLine();

	// 通过 ProcessEvent 调用 UKismetSystemLibrary::LineTraceSingle（替代失效的 pattern 直接调用）
	bool CallLineTraceSingle(void* WorldContextObject, const vec3& start, const vec3& end,
		void* const* actorsToIgnore, const int actorCount, const int traceChannel, FHitResult& outHit);

	// 通过 ProcessEvent 调用任意 UFunction（obj=调用对象, func=UFunction, params=参数缓冲区）
	// 前置条件：InitTraceLine() 已成功（内部完成 GObjects/名称解析/ProcessEvent 绑定）
	bool CallUFunction(void* obj, void* func, void* params);
}

vec2 CalcAngle(const vec3& src, const vec3& dst, const vec2 currentAngles, float smoothness);

// ============ 崩溃诊断：日志 + 步骤标记 + VEH 崩溃捕获 ============
// 日志位置：%TEMP%\sws_trainer.log（C:\ 根目录无权限写，改用 TEMP 必定可写）
// 崩溃时 VEH 会自动记录异常代码、崩溃地址（模块+RVA）与最后执行步骤
namespace dbg
{
	// 追加一行日志（自动带时间戳）
	void Log(const char* fmt, ...);

	// 标记当前执行步骤（相同步骤只记一次；变化时节流写入日志）
	void SetStep(const char* name);

	// 安装 VEH 崩溃捕获（Init 时调用一次）
	void InstallCrashHandler();

	// 当前执行步骤（崩溃时记录，用于定位）
	inline const char* LastStep = "";
}

// 在关键函数入口标记当前步骤
#define DBG_STEP(name) dbg::SetStep(name)

vec3 GetVectorForward(const vec3& angles);

template<const byte actorsToIgnoreCount = 0>
__forceinline bool TraceLine(const vec3& src, const vec3& dst, FHitResult& hitinfo, const std::array<void*, actorsToIgnoreCount>& actorsToIgnore = {}, const int traceChannel = 0)
{
	return mem::CallLineTraceSingle(
		UWorld,                                  // WorldContextObject
		src, dst,                                // Start / End
		actorsToIgnore.data(),                   // ActorsToIgnore
		(int)actorsToIgnoreCount,
		traceChannel,
		hitinfo);                                // OutHit
}
