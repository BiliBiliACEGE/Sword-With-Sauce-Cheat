#include "menu.h"
#include "../includes.h"
#include "../cfg.hpp"
#include "../cheat/UnrealEngine/UnrealClasses.h"
#include <string>



namespace ImGui
{
	#define IsKeyPressed(key) (GetAsyncKeyState(key) & 1 && GetAsyncKeyState(key) & 0x8000)

	void HotkeyButton(USHORT& dst, const char* const label, const char* new_button_id, bool& get_key);

	void DrawSeparator(const USHORT& extend);
}



// MENU
namespace Menu
{
	inline constexpr const vec2 MenuSize = { 800, 650 };


	void Render()
	{
		// MENU
		if (cfg->menu->bShowMenu)
		{
			static bool bInited = false;

			if (!bInited)
			{
				ImGui::GetStyle().WindowTitleAlign = { 0.5f, 0.5f };
				ImGui::SetNextWindowPos(ImVec2(0, 0));
				ImGui::SetNextWindowSize(MenuSize);

				bInited = true;
			}

			ImGui::Begin(u8"Sword With Sauce 2.4.0 | ���ײ˵�", 0,
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoResize);

			ImGui::Columns(2);
			ImGui::SetColumnOffset(1, 420);

			// ESP
			ImGui::Text(u8"͸�ӹ���");
			ImGui::DrawSeparator(MenuSize.x - 15);
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Checkbox(u8"���˱߿�", &cfg->esp->bBoxes);
			ImGui::SameLine();
			static constexpr const char* const Items_BoxTypes[] = { u8"2D�߿�", u8"3D�߿�", u8"�ǵ�߿�" };
			ImGui::SetNextItemWidth(150);
			assert(cfg->esp->boxType >= 0 && cfg->esp->boxType < IM_ARRAYSIZE(Items_BoxTypes));
			ImGui::Combo(u8"�߿�����", &cfg->esp->boxType, Items_BoxTypes, IM_ARRAYSIZE(Items_BoxTypes));

			ImGui::Checkbox(u8"͸����", &cfg->esp->bSnaplines);
			ImGui::Checkbox(u8"���˹Ǽ�", &cfg->esp->bSkeleton);

			// TRIGGERBOT
			ImGui::SetCursorPosY(210);
			ImGui::Text(u8"���˴�����");
			ImGui::DrawSeparator(MenuSize.x - 15);
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Checkbox(u8"����", &cfg->trg->bEnabled);

			static bool bGetKey1 = false;
			ImGui::HotkeyButton(cfg->trg->key, u8"�ȼ�", "##19f212", bGetKey1);

			ImGui::Checkbox(u8"���", &cfg->trg->bDelay);
			if (cfg->trg->bDelay)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				ImGui::Combo(u8"���ģʽ", &cfg->trg->delayMode, u8"�Զ���\0���\0\0");

				ImGui::Spacing();
				if (!cfg->trg->delayMode) // custom delay
				{
					ImGui::SetNextItemWidth(170);
					ImGui::SliderInt("Delay##slider", &cfg->trg->customDelay, 1, 300, u8"%d ����");
				}
				else // random delay
				{
					ImGui::SetNextItemWidth(150);
					ImGui::SliderInt(u8"��С Delay##slider", &cfg->trg->minRanDelay, 1, 300, u8"%d ����");
					ImGui::SetNextItemWidth(150);
					ImGui::SliderInt(u8"��� Delay##slider", &cfg->trg->maxRanDelay, 2, 300, u8"%d ����");

					if (cfg->trg->maxRanDelay <= cfg->trg->minRanDelay)
						cfg->trg->minRanDelay = cfg->trg->maxRanDelay - 1;
				}
			}

			ImGui::SetCursorPos({ 0, MenuSize.y - 5 });
			ImGui::NextColumn();

			// AIMBOT
			ImGui::Text(u8"ʵ�ù���");
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Checkbox(u8"�Զ�����", &cfg->aimbot->bEnabled);

			ImGui::Checkbox(u8"�ɼ��ȼ��", &cfg->aimbot->bVisibleOnly);

			ImGui::Checkbox(u8"��Ұ", &cfg->aimbot->bFOV);
			if (cfg->aimbot->bFOV)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(150);
				ImGui::SliderInt("##fov slider", &cfg->aimbot->FOV, 4, winSize.x);

				ImGui::Checkbox(u8"��Ұ��Χ", &cfg->aimbot->bFOVCircle);
				ImGui::Dummy(ImVec2(0, 10)); // ��� 20 ���ش�ֱ���
			}


			

			/*static bool bGetKey2 = false;
			ImGui::HotkeyButton(cfg->aimbot->aimKey, u8"�ȼ�", "##29r9102", bGetKey2);
			ImGui::Spacing();*/

			// EXPLOITS
			ImGui::SetCursorPosY(210);
			ImGui::Dummy(ImVec2(0, 40)); 
			ImGui::Checkbox(u8"��������", &cfg->exploit->bInfiniteHealth);
			ImGui::Checkbox(u8"���޵�ҩ", &cfg->exploit->bInfiniteAmmo);
			ImGui::Checkbox(u8"����Ⱥ��", &cfg->exploit->bNoSpread);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(u8"��Ĳ������ã�����...");
			ImGui::Checkbox(u8"��ǽ", &cfg->exploit->bNoClip);
			ImGui::SameLine();
			static bool bGetKey3 = false;
			ImGui::HotkeyButton(cfg->exploit->noClipKey, u8"�ȼ�", "##19r892", bGetKey3);

			ImGui::Checkbox(u8"����ģʽ", &cfg->exploit->bFlyHack);
			ImGui::SameLine();
			static bool bGetKey4 = false;
			ImGui::HotkeyButton(cfg->exploit->flyKey, u8"�ȼ�", "##9591", bGetKey4);

			ImGui::Checkbox(u8"�޸��ٶ�", &cfg->exploit->bSpeedHack);
			if (cfg->exploit->bSpeedHack)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(130);
				ImGui::SliderFloat("##speed slider", &cfg->exploit->speed, 1000.f, 5000.f, "%.2f");
			}

			ImGui::Checkbox(u8"������Ծ", &cfg->exploit->bSuperJump);
			if (cfg->exploit->bSuperJump)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(130);
				ImGui::SliderFloat("##jumpHeight slider", &cfg->exploit->jumpHeight, 420.f, 1000.f, "%.2f");
			}

			ImGui::Checkbox(u8"�޸�����", &cfg->exploit->bModGravity);
			if (cfg->exploit->bModGravity)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(120);
				ImGui::SliderFloat("##modGravity slider", &cfg->exploit->modGravity, 0.f, 1.f, "%.2f");
			}
			ImGui::SetNextItemWidth(160);
			ImGui::SliderFloat(u8"�ӽ�ƽ��", &cfg->aimbot->smoothness, 1.f, 10.f, "%.2f");

			ImGui::End();
		}
	}
}

namespace ImGui
{
	void HotkeyButton(USHORT& dst, const char* const label, const char* new_button_id, bool& get_key)
	{
		#define XorCStr(x) x

		std::string button;

		if (get_key)
			button += "...";
		else
		{
			switch (dst)
			{
			case 0:
				button += XorCStr("NONE");
				break;
			case VK_ESCAPE:
				button += XorCStr("ESC");
				break;
			case VK_TAB:
				button += XorCStr("TAB");
				break;
			case VK_CAPITAL:
				button += XorCStr("CAPS");
				break;
			case VK_SHIFT:
				button += XorCStr("SHIFT");
				break;
			case VK_CONTROL:
				button += XorCStr("CTRL");
				break;
			case VK_MENU:
				button += XorCStr("ALT");
				break;
			case VK_XBUTTON1:
				button += XorCStr("MB 1");
				break;
			case VK_XBUTTON2:
				button += XorCStr("MB 2");
				break;
			case VK_F1:
				button += XorCStr("F1");
				break;
			case VK_F2:
				button += XorCStr("F2");
				break;
			case VK_F3:
				button += XorCStr("F3");
				break;
			case VK_F4:
				button += XorCStr("F4");
				break;
			case VK_F5:
				button += XorCStr("F5");
				break;
			case VK_F6:
				button += XorCStr("F6");
				break;
			case VK_F7:
				button += XorCStr("F7");
				break;
			case VK_F8:
				button += XorCStr("F8");
				break;
			case VK_F9:
				button += XorCStr("F9");
				break;
			case VK_F10:
				button += XorCStr("F10");
				break;
			case VK_F11:
				button += XorCStr("F11");
				break;
			case VK_F12:
				button += XorCStr("F12");
				break;
			case VK_F13:
				button += XorCStr("F13");
				break;
			case VK_F14:
				button += XorCStr("F14");
				break;
			case VK_F15:
				button += XorCStr("F15");
				break;
			case VK_F16:
				button += XorCStr("F16");
				break;
			case VK_F17:
				button += XorCStr("F17");
				break;
			case VK_F18:
				button += XorCStr("F18");
				break;
			case VK_F19:
				button += XorCStr("F19");
				break;
			case VK_F20:
				button += XorCStr("F20");
				break;
			case VK_F21:
				button += XorCStr("F21");
				break;
			case VK_F22:
				button += XorCStr("F22");
				break;
			case VK_F23:
				button += XorCStr("F23");
				break;
			case VK_F24:
				button += XorCStr("F24");
				break;
			case VK_HOME:
				button += XorCStr("HOME");
				break;
			case VK_DELETE:
				button += XorCStr("DELETE");
				break;
			case VK_INSERT:
				button += XorCStr("INSERT");
				break;
			case VK_END:
				button += XorCStr("END");
				break;
			case VK_SPACE:
				button += XorCStr("SPACE");
				break;
			case VK_SCROLL:
				button += XorCStr("SCROLL");
				break;
			case VK_LBUTTON:
				button += XorCStr("LCLICK");
				break;
			case VK_RBUTTON:
				button += XorCStr("RCLICK");
				break;
			case VK_RETURN:
				button += XorCStr("ENTER");
				break;
			default:
				button += dst;
			}
		}
		button += new_button_id;

		if (ImGui::Button(button.c_str(), ImVec2(75, 0)))
			get_key = true;
		if (ImGui::IsItemHovered())
		{
			if (dst != 0xADAF)
				ImGui::SetTooltip(XorCStr(u8"˫������"));
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				get_key = false;
				dst = 0;
			}
		}

		ImGui::SameLine();
		ImGui::Text(label);

		if (get_key)
		{
			for (int i = 0x1; i <= 0x100; i += 0x1)
			{
				if (GetAsyncKeyState(i) & 0x8000)
				{
					get_key = false;
					dst = i;
					break;
				}
			}
		}

		#undef XorCStr
	}

	void DrawSeparator(const USHORT& extend)
	{
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::GetForegroundDrawList()->AddLine(
			pos,																				// p1
			{ pos.x + extend, pos.y },															// p2
			(ImColor)ImGui::GetStyle().Colors[ImGuiCol_Separator],								// color
			1.f);																				// thickness
	}
}
