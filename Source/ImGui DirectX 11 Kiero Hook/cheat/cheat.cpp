#include "cheat.h"
#include "../includes.h"
#include "../cfg.hpp"
#include "../kiero/minhook/include/MinHook.h"
#include <ctime>



#define IsKeyHeld(key) (GetAsyncKeyState(key) & 0x8000)

// Dumper-7 导出的最新版本静态偏移（RVA），作为 pattern 扫描失败时的回退
#define GWORLD_FALLBACK_OFFSET 0x03048450

#define BONE_HEAD_ID 48
#define BONE_NECK_ID 47
#define BONE_CHEST_ID 4
#define BONE_ROOT_ID 0

/// <summary>
/// All signatures/patterns/AOBs used in this project will work for ALL games using Unreal Engine 4.16.3
/// The cheat should work for almost all Sword With Sauce versions since it uses patterns/signatures
/// </summary>

namespace Cheat
{
	DWORD GWorldOffset = 0;

	// 通过 World 名称检测是否处于主菜单/大厅（替代旧版硬编码的 MAIN_MENU_FNAME_INDEX 19867）
	__forceinline bool IsInMenuWorld()
	{
		if (!UWorld) return true;

		const char* worldName = mem::GetGName(UWorld->FNameIndex);
		if (!worldName || !worldName[0]) return true;

		return strstr(worldName, "Menu") != nullptr || strstr(worldName, "Lobby") != nullptr;
	}

	__forceinline bool UpdatePointers()
	{
		DBG_STEP("UpdatePointers");

		UWorld = *(_UWorld**)(ModuleBase + GWorldOffset);
		if (!UWorld) return false;
		if (!UWorld->GameInstance || !UWorld->PersistentLevel) return false;

		// 防御：LocalPlayers 数组为空时直接失败（避免越界读）
		auto& localPlayers = UWorld->GameInstance->LocalPlayers;
		if (!localPlayers.items || localPlayers.count <= 0) return false;

		LocalPlayer = localPlayers[0];
		if (!LocalPlayer || !LocalPlayer->PlayerController) return false;

		CameraManager = LocalPlayer->PlayerController->CameraManager;
		EntityList = &UWorld->PersistentLevel->EntityList;

		return true;
	}

	__forceinline bool WorldToScreen(const vec3& worldLoc, vec2& screenPos)
	{
		return CameraManager->WorldToScreen(worldLoc, screenPos);
	}

	__forceinline bool WorldToScreen(const vec3& worldLoc, vec3& screenPos)
	{
		return CameraManager->WorldToScreen(worldLoc, *(vec2*)&screenPos);
	}

	AEnemy* GetClosestEnemy(vec3& outLocation, int& outId)
	{
		AEnemy* ret = nullptr;
		float closestDist = INFINITY;

		for (USHORT i = 0; i < EntityList->count; i++)
		{
			auto& ent = EntityList->items[i];
			if (!ent->IsPlayer() || (cfg->aimbot->bVisibleOnly && !ent->IsVisible()))
				continue;

			vec3 target = ent->GetBonePos(BONE_HEAD_ID);

			vec2 screenPos;
			if (!WorldToScreen(target, screenPos))
				continue;

			float dist = winCenter.Distance(screenPos);
			if (cfg->aimbot->bFOV && dist > cfg->aimbot->FOV)
				continue;

			if (dist < closestDist)
			{
				closestDist = dist;
				outLocation = target;
				outId = ent->ObjectIndex;
				ret = ent;
			}
		}

		return ret;
	}

	bool bRun = false;
	void DrawESP()
	{
		DBG_STEP("DrawESP");

		if (!bRun || !EntityList || !EntityList->items) return;
		if (!EntityList || !EntityList->items) return;

		ImDrawList* draw = ImGui::GetBackgroundDrawList();

		// FOV CIRCLE
		if (cfg->aimbot->bEnabled && cfg->aimbot->bFOV && cfg->aimbot->bFOVCircle)
			draw->AddCircle(
				winCenter,									// center
				cfg->aimbot->FOV,							// radius
				ImColor(255, 0, 0),							// color
				cfg->aimbot->FOV / 50 + 30,					// segments
				cfg->aimbot->FOV / 80 + 1.f);				// thickness

		bool BoxOrSnapline = cfg->esp->bBoxes || cfg->esp->bSnaplines;
		if (!BoxOrSnapline && !cfg->esp->bSkeleton)
			return;

		for (USHORT i = 0; i < EntityList->count; i++)
		{
			auto& ent = EntityList->items[i];
			if (!ent) continue; // ������ָ��ʵ��

			DBG_STEP("DrawESP_EntityLoop");

			if (!ent->IsPlayer())
				continue;

			// SKELETON ESP
			if (cfg->esp->bSkeleton)
			{
				DBG_STEP("DrawESP_Skeleton");
				auto DrawBone = [&](const byte bone1, const byte bone2)
				{
					if (!ent) return; // ��ֹ�첽�ͷŵ��µĿ�ָ��
					vec2 screenPos1, screenPos2;

					bool
						b1 = WorldToScreen(ent->GetBonePos(bone1), screenPos1),
						b2 = WorldToScreen(ent->GetBonePos(bone2), screenPos2);

					if (!b1 && !b2)
						return;
					
					draw->AddLine(
						screenPos1,				// p1
						screenPos2,				// p2
						ImColor(255, 0, 0),		// color
						1.f);					// thickness
				};

				auto DrawSkeleton = [&]()
				{
					static constexpr const byte Skeleton_Struct[] = { // differs between player models and games
						// HEAD
						48, 47,

						// BACK
						47, 4,
						4, 3,
						3, 2,
						2, 1,

						// RIGHT ARM
						47, 27,
						27, 28,
						28, 29,
						29, 33,

						// LEFT ARM
						47, 6,
						6, 7,
						7, 8,
						8, 12,

						// RIGHT LEG
						1, 56,
						56, 58,
						58, 59,

						// LEFT LEG
						1, 50,
						50, 52,
						52,	53
					};
					static_assert(IM_ARRAYSIZE(Skeleton_Struct) % 2 == 0);

					for (byte j = 0; j < IM_ARRAYSIZE(Skeleton_Struct) - 1; j += 2)
						DrawBone(Skeleton_Struct[j], Skeleton_Struct[j + 1]);
				};

				DrawSkeleton();
			}

			if (BoxOrSnapline)
			{
				vec3 WHeadPos = ent->GetBonePos(BONE_HEAD_ID);
				WHeadPos.z += 5.f;

				vec2 headPos;
				if (!WorldToScreen(WHeadPos, headPos))
					continue;

				vec3 WFeetPos = ent->GetBonePos(BONE_ROOT_ID);

				vec2 feetPos;
				if (!WorldToScreen(WFeetPos, feetPos))
					continue;

				// SNAPLINE ESP
				if (cfg->esp->bSnaplines)
				{
					draw->AddLine(
						{ winCenter.x, winSize.y - 5 }, // p1
						feetPos,						// p2
						ImColor(255, 0, 0),				// color
						1.f);							// thickness
				}

				// BOX ESP
				if (cfg->esp->bBoxes)
				{
					switch (cfg->esp->boxType)
					{
					// 2D BOX
					case 0:
					{
						float boxWidth = sqrtf((headPos.y - feetPos.y) * (headPos.y - feetPos.y)) / 4;

						draw->AddRect(
							{ feetPos.x - boxWidth, headPos.y }, // tl
							{ feetPos.x + boxWidth, feetPos.y }, // br
							ImColor(255, 0, 0),					 // color
							0, 0,								 // rounding
							1.f);								 // thickness
					}
					break;

					// 3D BOX
					case 1:
					{
						static constexpr const vec3 boxRescaling = { 55.f /*thickness*/, 5.f /*top*/, 5.f /*bottom*/ };

						vec3 b1, b2, b3, b4, t1, t2, t3, t4;
						b1.z = b2.z = b3.z = b4.z = WFeetPos.z - boxRescaling.z;

						float yaw = ent->GetYaw();
						#define TORAD(x) ((x) * (M_PI / 180.f))

						b1.x = WFeetPos.x + cosf(TORAD(yaw + 45.f)) * boxRescaling.x;
						b1.y = WFeetPos.y + sinf(TORAD(yaw + 45.f)) * boxRescaling.x;

						b2.x = WFeetPos.x + cosf(TORAD(yaw + 135.f)) * boxRescaling.x;
						b2.y = WFeetPos.y + sinf(TORAD(yaw + 135.f)) * boxRescaling.x;

						b3.x = WFeetPos.x + cosf(TORAD(yaw + 225.f)) * boxRescaling.x;
						b3.y = WFeetPos.y + sinf(TORAD(yaw + 225.f)) * boxRescaling.x;

						b4.x = WFeetPos.x + cosf(TORAD(yaw + 315.f)) * boxRescaling.x;
						b4.y = WFeetPos.y + sinf(TORAD(yaw + 315.f)) * boxRescaling.x;

						#undef TORAD

						t1 = b1, t2 = b2, t3 = b3, t4 = b4;
						t1.z = t2.z = t3.z = t4.z = WHeadPos.z + boxRescaling.y;

						bool a = WorldToScreen(b1, b1);
						bool b = WorldToScreen(b2, b2);
						bool c = WorldToScreen(b3, b3);
						bool d = WorldToScreen(b4, b4);

						bool e = WorldToScreen(t1, t1);
						bool f = WorldToScreen(t2, t2);
						bool g = WorldToScreen(t3, t3);
						bool h = WorldToScreen(t4, t4);

						bool bottom = a && b && c && d;
						if (bottom)
						{
							// BOTTOM PART
							draw->AddLine(b1, b2, ImColor(255, 0, 0), 1.f);
							draw->AddLine(b2, b3, ImColor(255, 0, 0), 1.f);
							draw->AddLine(b3, b4, ImColor(255, 0, 0), 1.f);
							draw->AddLine(b4, b1, ImColor(255, 0, 0), 1.f);

							// COLUMNS TO TOP
							draw->AddLine(b1, t1, ImColor(255, 0, 0), 1.f);
							draw->AddLine(b2, t2, ImColor(255, 0, 0), 1.f);
							draw->AddLine(b3, t3, ImColor(255, 0, 0), 1.f);
							draw->AddLine(b4, t4, ImColor(255, 0, 0), 1.f);
						}

						// TOP PART
						bool top = e && f && g && h;
						if (top)
						{
							// TOP PART
							draw->AddLine(t1, t2, ImColor(255, 0, 0), 1.f);
							draw->AddLine(t2, t3, ImColor(255, 0, 0), 1.f);
							draw->AddLine(t3, t4, ImColor(255, 0, 0), 1.f);
							draw->AddLine(t4, t1, ImColor(255, 0, 0), 1.f);

							// COLUMNS TO TOP
							if (!bottom)
							{
								draw->AddLine(t1, b1, ImColor(255, 0, 0), 1.f);
								draw->AddLine(t2, b2, ImColor(255, 0, 0), 1.f);
								draw->AddLine(t3, b3, ImColor(255, 0, 0), 1.f);
								draw->AddLine(t4, b4, ImColor(255, 0, 0), 1.f);
							}
						}
					}
					break;

					// CORNERS
					case 2:
					{
						float dist = sqrtf((headPos.y - feetPos.y) * (headPos.y - feetPos.y));
						float boxWidth = dist / 4;
						float cornerSize = dist / 5;

						vec2 tl = { headPos.x - boxWidth, headPos.y };
						vec2 br = { headPos.x + boxWidth, feetPos.y };

						// TOP LEFT
						draw->AddLine({ tl.x, tl.y }, { tl.x, tl.y + cornerSize }, ImColor(255, 0, 0), 1.f);
						draw->AddLine({ tl.x, tl.y }, { tl.x + cornerSize, tl.y }, ImColor(255, 0, 0), 1.f);

						// TOP RIGHT
						draw->AddLine({ br.x, tl.y }, { br.x, tl.y + cornerSize }, ImColor(255, 0, 0), 1.f);
						draw->AddLine({ br.x, tl.y }, { br.x - cornerSize, tl.y }, ImColor(255, 0, 0), 1.f);

						// BOTTOM RIGHT
						draw->AddLine({ br.x, br.y }, { br.x, br.y - cornerSize }, ImColor(255, 0, 0), 1.f);
						draw->AddLine({ br.x, br.y }, { br.x - cornerSize, br.y }, ImColor(255, 0, 0), 1.f);

						// BOTTOM LEFT
						draw->AddLine({ tl.x, br.y }, { tl.x, br.y - cornerSize }, ImColor(255, 0, 0), 1.f);
						draw->AddLine({ tl.x, br.y }, { tl.x + cornerSize, br.y }, ImColor(255, 0, 0), 1.f);
					}
					break;
					}
				}
			}
		}
	}

	void Aimbot()
	{
		auto AimAtPos = [](const vec3& worldLocation)
		{
			// implementing my velocity, distance to enemy as well as the smoothness in the calculations for a more accurate aimbot
			vec3 src = CameraManager->Location
						+ (LocalPlayer->PlayerController->Character->GetVelocity()
						* CameraManager->Location.Distance(worldLocation)
						/ 49650.f
						* cfg->aimbot->smoothness); 

			vec2 angle = CalcAngle(
				src,							// source world location
				worldLocation,					// target world location
				CameraManager->Rotation,		// current angles/rotation
				cfg->aimbot->smoothness);		// smoothness

			LocalPlayer->PlayerController->ControlRotation = angle;
		};

		static int foundEntId = 0;
		if (!cfg->aimbot->bEnabled || !IsKeyHeld(cfg->aimbot->aimKey))
		{
			foundEntId = 0;
			return;
		}

		// IF ALREADY TARGET FOUND
		if (foundEntId)
		{
			for (USHORT i = 0; i < EntityList->count; i++)
			{
				auto& ent = EntityList->items[i];
				if (ent->IsPlayer() && ent->ObjectIndex == foundEntId)
				{
					if (cfg->aimbot->bVisibleOnly && !ent->IsVisible())
						break;
					AimAtPos(ent->GetBonePos(BONE_HEAD_ID));
					return;
				}
			}
			foundEntId = 0;
		}

		// FIND NEW TARGET
		vec3 closestEnt{};
		if (GetClosestEnemy(closestEnt, foundEntId))
			AimAtPos(closestEnt);
	}

	void Triggerbot(const float deltaTime)
	{
		static bool bShooting = false;
		static bool bDetected = false;
		static float delayCounter = 0.f;

		if (!cfg->trg->bEnabled || !IsKeyHeld(cfg->trg->key))
		{
			if (bShooting)
			{
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				bShooting = false;
			}

			delayCounter = 0.f;
			return;
		}

		if (bDetected)
		{
			if (delayCounter > 0.f)
				delayCounter -= deltaTime * 1000.f; // delta time is in seconds and multiplying it be 1000 will give us ms; delta time is almost always below 1.f because the frame updates a few times per second
			else
			{
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				delayCounter = 0.f;
				bDetected = false;
				bShooting = true;
			}
		}
		else
		{
			vec3 src = CameraManager->Location;
			vec3 dst = src + GetVectorForward(CameraManager->Rotation) * 99999999.f;

			FHitResult hit;
			switch (TraceLine<1>(src, dst, hit, { LocalPlayer->PlayerController->Character })) // using a switch statement so I can break out of the statement, the if statement doesn't have that option
			{
			case true:
				AEnemy* actor = hit.GetActor();
				if (!actor || !actor->HasValidId()) break;

				std::string name = mem::GetGName(actor->FNameIndex);

				auto OnNewEntFound = []()
				{
					if (!bShooting)
					{
						if (cfg->trg->bDelay)
						{
							bDetected = true;
							if (!cfg->trg->delayMode) // custom delay
							{
								delayCounter = float(cfg->trg->customDelay);
							}
							else // random delay
							{
								srand(time(0));
								delayCounter = float(rand() % (cfg->trg->maxRanDelay - cfg->trg->minRanDelay) + cfg->trg->minRanDelay);
							}
						}
						else
						{
							mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
							bShooting = true;
							delayCounter = 0;
						}
					}
					else
					{
						mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					}
				};

				// FOUND AN ENEMY IN OUR CROSSHAIR
				if (name._Starts_with("EnemyAI_BluePrint"))
				{
					if (actor->EnemyState)
					{
						OnNewEntFound();
						break;
					}
				}

				// FOUND GLASS IN OUR CROSSHAIR
				else if (name._Starts_with("Glass")) // maybe we hit glass
				{
					if (TraceLine<1>(hit.ImpactPoint, dst, hit, { actor }, 1)) // pass in 1 as the trace channel because for whatever reason the Glass actor on the 0 trace channel doesn't get ignored although I pass it as an argument to the function
					{
						actor = hit.GetActor();
						if (actor->IsPlayer())
						{
							OnNewEntFound();
							break;
						}
					}
				}

				// if we didn't find any enemy in our crosshair, we reset
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				bShooting = false;
				delayCounter = 0.f;
			}
		}
	}

	// 消灭群体：通过游戏原生 Damage 蓝图函数（反射调用）击杀全部存活敌人
	// 死亡逻辑（尸体/音效/计分）全部由游戏自身处理
	void KillAllEnemies()
	{
		// 边沿触发：勾选瞬间执行一次（重复勾选需先取消再勾选）
		static bool bPrevOn = false;
		const bool bNowOn = cfg->exploit->bNoSpread;
		const bool bTrigger = bNowOn && !bPrevOn;
		bPrevOn = bNowOn;
		if (!bTrigger || !EntityList || !EntityList->items)
			return;

		// 查找 EnemyAI_BluePrintV2_C::Damage UFunction（缓存）
		static void* DamageFunc = nullptr;
		if (!DamageFunc)
		{
			if (!mem::InitTraceLine())
			{
				dbg::Log("[KILL] 反射初始化失败");
				return;
			}

			UObject* enemyClass = mem::FindObjectByName("EnemyAI_BluePrintV2_C");
			if (!enemyClass)
			{
				dbg::Log("[KILL] 未找到 EnemyAI_BluePrintV2_C 类");
				return;
			}

			DamageFunc = mem::FindClassFunction(enemyClass, "Damage");
			if (!DamageFunc)
			{
				dbg::Log("[KILL] 未找到 Damage 函数");
				return;
			}
			dbg::Log("[KILL] Damage 函数绑定成功 0x%llX", (unsigned long long)DamageFunc);
		}

		// EnemyAI_BluePrintV2_C::Damage 参数布局（Dumper-7 parameters.hpp，总大小 0x60）
		struct DamageParams
		{
			float Damage_0;      // 0x00 伤害值
			bool HeadShotCheck;  // 0x04
			char pad1[0x3];      // 0x05
			vec3 Impulse;        // 0x08 冲量
			int BoneNameIdx;     // 0x14 FName.ComparisonIndex（0 = "None"）
			int BoneNameNum;     // 0x18 FName.Number
			bool Blood;          // 0x1C
			char pad2[0x3];      // 0x1D
			vec3 Location;       // 0x20 效果位置
			vec3 Direction;      // 0x2C
			byte excutionkill;   // 0x38 处决类型枚举
			char pad3[0x7];      // 0x39
			void* Actor;         // 0x40 伤害来源
			byte DamageSource;   // 0x48 伤害来源枚举
			char pad4[0x7];      // 0x49
			void* TagStr;        // 0x50 FString.Data（null = 空字符串）
			int TagCount;        // 0x58
			int TagMax;          // 0x5C
		};
		static_assert(sizeof(DamageParams) == 0x60, "Damage 参数布局与最新版本不匹配");

		int killed = 0, skipped = 0;
		for (USHORT i = 0; i < EntityList->count; i++)
		{
			AEnemy* ent = EntityList->items[i];
			if (!ent || !ent->IsPlayer())
				continue;

			if (ent->Health <= 0.f) // 已死亡跳过
			{
				skipped++;
				continue;
			}

			DamageParams params = {};
			params.Damage_0 = 1e9f;                                            // 一击必杀
			params.Location = ent->GetBonePos(BONE_ROOT_ID);                   // 效果作用于敌人位置
			params.Actor = LocalPlayer->PlayerController->Character;           // 伤害来源 = 玩家

			if (mem::CallUFunction(ent, DamageFunc, &params))
				killed++;
		}

		dbg::Log("[KILL] 击杀 %d 个敌人（跳过已死亡 %d 个）", killed, skipped);
	}

	void Exploits()
	{
		auto player = LocalPlayer->PlayerController->Character;

		// INFITE AMMO (最新版本弹药存储于 Extra_Resources TMap 中的 UExtraResource_Object_C::Ammo_Int)
		if (cfg->exploit->bInfiniteAmmo)
		{
			player->SetInfiniteAmmo(999);
		}
		// INFINITE HEALTH
		if (cfg->exploit->bInfiniteHealth)
			player->Health = 999999.f;
		// KILL ALL ENEMIES（消灭群体：边沿触发，见 KillAllEnemies）
		KillAllEnemies();

		// NO CLIP
		static bool bResetNoClip = false;
		if (cfg->exploit->bNoClip)
		{
			if (!bResetNoClip) // we need to set the value only once per toggle!
			{
				player->bActorEnableCollision &= ~2; // Actor.bActorEnableCollision.Bit = 2
				bResetNoClip = true;
			}
		}
		else if (bResetNoClip)
		{
			player->bActorEnableCollision |= 2; // Actor.bActorEnableCollision.Bit = 2
			bResetNoClip = false;
		}

		// FLY HACK
		static bool bResetFly = false;
		if (cfg->exploit->bFlyHack)
		{
			player->CharacterMovement->MovementMode = ACharacterMovementComponent::MOVE_Flying; // set to fly mode so we don't fall down

			vec3 sum = { };
			vec3 newRot = { };
			float flySpeed = player->CharacterMovement->MaxFlySpeed / 90.f;

			if (IsKeyHeld(VK_SHIFT))
				flySpeed *= 1.5f; // hold shift to speed fly

			if (IsKeyHeld('W'))
				sum += GetVectorForward(CameraManager->Rotation) * flySpeed;
			else if (IsKeyHeld('S')) // we dont want to move backwards if we are holding W
			{
				newRot = {
					-CameraManager->Rotation.x,		   // when goind backwards, we need to flip the value here
					CameraManager->Rotation.y + 180.f, // + 180.f so we look behind us, 'S' is for going backwards after all...
					0.f								   // roll is always zero
				};
				sum += GetVectorForward(newRot) * flySpeed;
			}

			if (IsKeyHeld('D'))
			{
				newRot = {
					0.f,							  // when going left or right we dont't want to move up or down
					CameraManager->Rotation.y + 90.f, // + 90.f so we look to the right, 'D' is for going right after all...
					0.f								  // roll is always zero
				};
				sum += GetVectorForward(newRot) * flySpeed;
			}
			else if (IsKeyHeld('A')) // we dont want to move left if we are holding A
			{
				newRot = {
					0.f,							   // when going left or right we dont't want to move up or down
					CameraManager->Rotation.y + 270.f, // + 270.f so we look to the left, 'A' is for going left after all...
					0.f								   // roll is always zero
				};
				sum += GetVectorForward(newRot) * flySpeed;
			}

			if (IsKeyHeld(VK_LCONTROL)) // for extra control over our character...
				sum.z -= flySpeed;
			else if (IsKeyHeld(VK_SPACE)) // don't go up if we are holding CTRL
				sum.z += flySpeed;

			player->CapsuleComponent->ModifiableLocation += sum;
			bResetFly = true;
		}
		else if (bResetFly)
		{
			player->CharacterMovement->MovementMode = ACharacterMovementComponent::MOVE_Falling;
			bResetFly = false;
		}

		// SPPEDHACK
		if (cfg->exploit->bSpeedHack)
		{
			player->CharacterMovement->MaxAcceleration = cfg->exploit->speed;
			player->CharacterMovement->MaxWalkSpeed = cfg->exploit->speed;
			player->CharacterMovement->MaxFlySpeed = cfg->exploit->speed;
		}

		// SUPER JUMP
		static bool bResetSuperJump = false;
		if (cfg->exploit->bSuperJump)
		{
			player->CharacterMovement->JumpZVelocity = cfg->exploit->jumpHeight;
			bResetSuperJump = true;
		}
		else if (bResetSuperJump)
		{
			player->CharacterMovement->JumpZVelocity = 420.f; // 420.f is the default in Sword With Sauce
			bResetSuperJump = false;
		}

		// MODIFY GRAVITY
		static bool bResetModGravity = false;
		if (cfg->exploit->bModGravity)
		{
			player->CharacterMovement->GravityScale = cfg->exploit->modGravity;
			bResetModGravity = true;
		}
		else if (bResetModGravity)
		{
			player->CharacterMovement->GravityScale = 1.f;
			bResetModGravity = false;
		}

		// safety check if we are above/under the map
		if (player->CapsuleComponent->ModifiableLocation.z > 10000.f) player->CapsuleComponent->ModifiableLocation.z = 10000.f;
		else if (player->CapsuleComponent->ModifiableLocation.z < 0.f) player->CapsuleComponent->ModifiableLocation.z = 0.f;
	}
		
	// 主循环：由 Present hook 每帧驱动（旧版 hook 的 APlayerController::PostProcessInput pattern 已在最新版本失效）
	void RunTick(const float DeltaTime)
	{
		DBG_STEP("RunTick");

		if (!UpdatePointers() || IsInMenuWorld() || !LocalPlayer->PlayerController->Character)
		{
			bRun = false;
			return;
		}

		bRun = true;

		// 首次进入游戏世界时记录一次
			static bool bLoggedInGame = false;
			if (!bLoggedInGame)
			{
				bLoggedInGame = true;
				dbg::Log("Entered game world (EntityList count=%d)", (int)EntityList->count);
			}

		DBG_STEP("Aimbot");
		Aimbot();
		DBG_STEP("Triggerbot");
		Triggerbot(DeltaTime);
		DBG_STEP("Exploits");
		Exploits();
	}

	__forceinline bool HOOK(const char* pattern, void* pDetour, void* pOriginal, const bool usedIdaStyleMask = true)
	{
		void* pFunc = (void*)mem::PatternScan(usedIdaStyleMask, pattern);
		if (!pFunc) return false;

		MH_CreateHook(pFunc, pDetour, (void**)pOriginal);
		MH_EnableHook(pFunc);
		return true;
	}

	bool Init()
	{
		if (MH_Initialize() != MH_OK)
		{
			MessageBoxA(0, "Couldn't intialize MinHook!", "1hAck Trainer", MB_ICONERROR);
			return false;
		}

		ModuleBase = (uintptr_t)GetModuleHandleA(0); // gets the image base of the executable you inject the dll into
		ModuleBaseEnd = ModuleBase + mem::GetModuleSize(0);

		// 崩溃捕获 + 日志（必须在其他初始化之前装好，才能覆盖整个生命周期）
		dbg::InstallCrashHandler();

		dbg::Log("Init start: ModuleBase=0x%llX Size=0x%llX", ModuleBase, ModuleBaseEnd - ModuleBase);

		// WINDOW SIZE: 由 hkPresent 每帧从 ImGui DisplaySize 更新（旧 pattern 已失效）

		// GET NEEDED OFFSETS
		GWorldOffset = mem::GetStaticOffset(3, 7, true, "48 8B 1D ?? ?? ?? ?? 48 8B 75 28 E8 ?? ?? ?? ?? 4C 8B C8 48 8D 4D 28 48 8B D3 4D 8B C7 FF 56 08");
		if (!GWorldOffset)
			GWorldOffset = GWORLD_FALLBACK_OFFSET; // 最新版本的静态偏移（Dumper-7 导出），pattern 失效时使用
		if (!GWorldOffset)
		{
			MessageBoxA(0, "Couldn't find GWorld!", "1hAck Trainer", MB_ICONERROR);
			return false;
		}
		dbg::Log("GWorldOffset=0x%X", GWorldOffset);

		// 名字解析：调用游戏 FName::AppendString（RVA 0x5FFCA0，profile 与 Dumper-7 双重确认）。
		// 已放弃 GNames 偏移方案：4.22.3 TNameEntryArray 布局无法可靠静态验证（假阳性骗过运行时检查导致崩溃），
		// 直接调用游戏自身函数与 Dumper-7 同款，版本无关且无需任何 GNames 偏移
		if (!mem::InitNameResolver())
		{
			MessageBoxA(0, "Couldn't init FName::AppendString resolver!", "1hAck Trainer", MB_ICONERROR);
			return false;
		}
		dbg::Log("Name resolver OK (AppendString RVA 0x%X)", mem::APPENDSTRING_RVA);

		// 初始化 LineTraceSingle 反射调用（旧 pattern 已失效，改为 GObjects + ProcessEvent 版本无关方案）
		if (!mem::InitTraceLine())
		{
			MessageBoxA(0, "Couldn't init UKismetSystemLibrary::LineTraceSingle!", "1hAck Trainer", MB_ICONERROR);
			return false;
		}
		dbg::Log("InitTraceLine OK");

		// 主循环不再 hook PostProcessInput（pattern 已失效），改为在 Present hook 中每帧调用 RunTick
		dbg::Log("Init complete");

		return true;
	}

	void Uninit()
	{
		MH_Uninitialize();
	}
}
