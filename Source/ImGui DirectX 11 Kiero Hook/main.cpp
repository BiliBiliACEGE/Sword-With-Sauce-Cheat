#include <thread>

#include "includes.h"
#include "cfg.hpp"
#include "cheat/cheat.h"
#include "menu/menu.h"


/// <summary>
/// 
/// *********************
///  GITHUB REPOSITORY: https://github.com/1hAck-0/UE4-Cheat-Source-Code
/// *********************
/// 
/// *********************
/// BUILD and COMPILE:
/// *********************
/// - You can only compile in VS22 or higher since the project was created with this version of Visual Studio. For ya all lazy asses who haven't updated from VS19, it's worth it anyway
/// - Build and compile in x64 architecture since Sword With Suace is 64-bit
/// - If you are still not able to compile, please put the correct path to the DirectX SDK in project settings->Configuration Properties->VC++ Directories->Include Directories and Library Directories
/// *********************
/// 
/// 
/// 
/// *********************
/// TO DO:
/// *********************
/// - Silent Aim
/// - Consider the Enemy velocity in the Aimbot calculations for more accurate aimbot/silent aim (I was able to get the velocity of the enemies but somewhy it made my aimbot very laggy)
/// - No Recoil/Spread
/// - View the first comment in FHitResult::GetActor() in UnrealEngine.cpp; why do we have to add 3 to get the actual serial number!?
/// 
/// If you know how to do anything in the TO DO list, please share you knowledge with a pull request in the GitHub Repo. Thanks!
/// *********************
/// 
/// 
/// 
/// *********************
/// FEATURES:
/// *********************
/// - ESP
///   - Boxes
///     - 2D Boxes
///     - 3D Boxes
///     - Corner Boxes
///   - Snaplines
///   - Skeletons
/// 
/// - Aimbot
///   - Visibile Check
///   - FOV + FOV Circle
///   - Smoothness
///   - Custom Hotkey
/// 
/// - Triggerbot
///   - Custom Hotkey
///   - Delay
///     - Specific Custom Delay
///     - Random Delay (you choose the range)
/// 
/// - Exploits
///   - Infinite Health
///   - Infinite Ammo
///   - No Spread (doesn't work well)
///   - No Clip (with custom toggle key option)
///	  - Flyhack (with custom toggle key option)
///   - Speedhack + Speed slider
///   - Super Jump + Jump Height slider
///   - Modify Gravity + Gravity slider
/// *********************
/// 
/// </summary>

#define IsKeyPressed(key) GetAsyncKeyState(key) & 1 && GetAsyncKeyState(key) & 0x8000

#pragma region Kiero

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Present oPresent;
HWND window = 0;
WNDPROC oWndProc;
ID3D11Device* pDevice = 0;
ID3D11DeviceContext* pContext = 0;
ID3D11RenderTargetView* mainRenderTargetView;


void InitImGui()
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGuiStyle& style = ImGui::GetStyle();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc",30.f,NULL,io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

#pragma endregion


// 重建 RenderTargetView（backbuffer 变化后调用）
static void RecreateRenderTargetView(IDXGISwapChain* pSwapChain)
{
	if (!pDevice)
		return;

	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)))
	{
		pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
		pBackBuffer->Release();
	}
}

// hook IDXGISwapChain::ResizeBuffers（vtable 索引 12）：
// 游戏运行中窗口尺寸/显示模式/输出格式变化时会调用本函数，D3D11 要求调用前
// 释放所有 backbuffer 引用，否则返回 DXGI_ERROR_INVALID_CALL 导致 UE4 LowLevelFatalError 崩溃。
// 我们持有的 mainRenderTargetView 与 context 上绑定的 RT 均引用 backbuffer，必须先释放再重建
typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
ResizeBuffers_t oResizeBuffers = nullptr;

HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (mainRenderTargetView)
	{
		mainRenderTargetView->Release();
		mainRenderTargetView = nullptr;
	}
	if (pContext) // 解绑 context 上的旧 RT，否则 context 仍持有 backbuffer 引用
		pContext->OMSetRenderTargets(0, nullptr, nullptr);

	const HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
	if (SUCCEEDED(hr))
		RecreateRenderTargetView(pSwapChain);

	return hr;
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!cfg->menu->bInit)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			RecreateRenderTargetView(pSwapChain);
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			cfg->menu->bInit = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	// 防御：RTV 尚未就绪（ResizeBuffers 重建失败）时跳过本帧绘制，防止使用失效视图崩溃
	if (!mainRenderTargetView)
		return oPresent(pSwapChain, SyncInterval, Flags);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DBG_STEP("Present_NewFrame");

	// 从 ImGui 获取窗口大小（替代旧版 pattern 扫描，兼容最新游戏版本且支持分辨率切换）
	const ImVec2& disp = ImGui::GetIO().DisplaySize;
	if (disp.x > 0.f && disp.y > 0.f)
	{
		winSize = { disp.x, disp.y };
		winCenter = winSize / 2;
	}
	if (IsKeyPressed(cfg->menu->ToggleKey))
		cfg->menu->bShowMenu = !cfg->menu->bShowMenu;

	DBG_STEP("Menu_Render");
	Menu::Render();

	// 主循环（每帧驱动，替代已失效的 PostProcessInput hook）
	Cheat::RunTick(ImGui::GetIO().DeltaTime);

	Cheat::DrawESP();

	DBG_STEP("ImGui_Render");
	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, 0);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

// ENTRY POINT
DWORD WINAPI Main(HMODULE hMod)
{
	if (!Cheat::Init())
	{
		FreeLibraryAndExitThread(hMod, 0);
		return 0;
	}

	// HOOK KIERO
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			// hook ResizeBuffers（vtable 索引 13，注意 12 是 GetDesc）：
			// backbuffer 重建（分辨率/窗口模式/输出格式变化）时
			// 先释放我们持有的 RTV 引用再调原函数，否则 DXGI_ERROR_INVALID_CALL 崩溃
			kiero::bind(13, (void**)&oResizeBuffers, hkResizeBuffers);
			init_hook = true;
		}
	} while (!init_hook);

	while (true)
	{
		// close menu
		if (IsKeyPressed(VK_F4) || cfg->menu->bShutdown)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// CLEAN UP
	cfg->menu->bShutdown = true;
	kiero::shutdown();
	Cheat::Uninit();
	FreeLibraryAndExitThread(hMod, 0);
	return 0;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Main, hMod, 0, nullptr);
		return TRUE;
	}
	return FALSE;
}
