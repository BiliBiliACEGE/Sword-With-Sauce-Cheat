#pragma once
#include "Classes.hpp"
#include <string>
#include <vector>



namespace mem
{
	uintptr_t GetPointerSafe(uintptr_t base, const std::vector<uintptr_t>& offsets);

	const char* GetGName(const int FNameIndex);
}

inline uintptr_t ModuleBase;
inline uintptr_t ModuleBaseEnd;
inline vec2 winSize;
inline vec2 winCenter;

struct UtilClass
{
#define DEFINE_MEMBER_N(type, name, offset) struct { char __pad__##name[offset]; type name; }
#define DEFINE_MEMBER_N0(type, name) struct { type name; }

	template<typename T> T read(const USHORT& offset) { return *(T*)((uintptr_t)this + offset); }
	template<typename T> void write(const USHORT& offset, const T& value) { return *(T*)((uintptr_t)this + offset) = value; }
};

template <typename T>
struct TArray
{
	inline T& operator[](const int i) { return items[i]; }

	inline const T& operator[](const int i) const { return items[i]; }

	__forceinline static TArray<T> Init(const std::vector<T>& items)
	{
		TArray<T> ret;
		ret.count = ret.max = items.size();
		ret.items = new T[ret.count];
		for (int i = 0; i < ret.count; i++)
			ret.items[i] = items[i];
		return ret;
	}

	__forceinline void Uninit()
	{
		delete[] this->items;
	}

	T* items = nullptr;
	int count = 0;
	int max = 0;
};

struct FString
{
	inline wchar_t* GetWString() { return this->str; }

	inline char* GetString()
	{
		std::wstring buf(this->str);
		char ret[256];
		strcpy_s(ret, std::string(buf.begin(), buf.end()).c_str());
		return ret;
	}

private:
	wchar_t* str;
	int count;
	int max;
};

struct FTransform
{
	vec4 Rotation;
	vec3 Translation;
	int _pad1;
	vec3 Scale;
	int _pad2;

	Matrix ToMatrixWithScale() const
	{
		Matrix m;
		m.m[3][0] = Translation.x;
		m.m[3][1] = Translation.y;
		m.m[3][2] = Translation.z;

		float x2 = Rotation.x + Rotation.x;
		float y2 = Rotation.y + Rotation.y;
		float z2 = Rotation.z + Rotation.z;

		float xx2 = Rotation.x * x2;
		float yy2 = Rotation.y * y2;
		float zz2 = Rotation.z * z2;
		m.m[0][0] = (1.0f - (yy2 + zz2)) * Scale.x;
		m.m[1][1] = (1.0f - (xx2 + zz2)) * Scale.y;
		m.m[2][2] = (1.0f - (xx2 + yy2)) * Scale.z;

		float yz2 = Rotation.y * z2;
		float wx2 = Rotation.w * x2;
		m.m[2][1] = (yz2 - wx2) * Scale.z;
		m.m[1][2] = (yz2 + wx2) * Scale.y;

		float xy2 = Rotation.x * y2;
		float wz2 = Rotation.w * z2;
		m.m[1][0] = (xy2 - wz2) * Scale.y;
		m.m[0][1] = (xy2 + wz2) * Scale.x;

		float xz2 = Rotation.x * z2;
		float wy2 = Rotation.w * y2;
		m.m[2][0] = (xz2 + wy2) * Scale.z;
		m.m[0][2] = (xz2 - wy2) * Scale.x;

		m.m[0][3] = 0.0f;
		m.m[1][3] = 0.0f;
		m.m[2][3] = 0.0f;
		m.m[3][3] = 1.0f;

		return m;
	}
}; // size: 0x30

struct FName
{
	union
	{
		struct
		{
			int32_t	ComparisonIndex;
			//int32_t DisplayIndex;
			uint32_t Number;
		};
		//uint64_t CompositeComparisonValue;
	};
};

struct TWeakObjectPtr
{
	int ObjectIndex;
	int ObjectSerialNumber;
};

// 最新版本 FHitResult 布局（Dumper-7 Engine_structs.hpp，总大小 0x88）
struct FHitResult
{
	uint32_t bBlockingHit : 1;      // 0x00
	uint32_t bStartPenetrating : 1; // 0x00
	int32_t FaceIndex;              // 0x04
	float Time;                     // 0x08
	float Distance;                 // 0x0C
	vec3 Location;                  // 0x10
	vec3 ImpactPoint;               // 0x1C
	vec3 Normal;                    // 0x28
	vec3 ImpactNormal;              // 0x34
	vec3 TraceStart;                // 0x40
	vec3 TraceEnd;                  // 0x4C
	float PenetrationDepth;         // 0x58
	int32_t Item;                   // 0x5C
	TWeakObjectPtr PhysMaterial;    // 0x60
	TWeakObjectPtr Actor;           // 0x68 (ObjectIndex + 0x68, SerialNumber + 0x6C)
	TWeakObjectPtr Component;       // 0x70
	FName BoneName;                 // 0x78
	FName MyBoneName;               // 0x80

	struct AEnemy* GetActor();
};
static_assert(sizeof(FHitResult) == 0x88, "FHitResult 布局与最新版本不匹配");

enum EStereoscopicPass
{
	eSSP_FULL,
	eSSP_LEFT_EYE,
	eSSP_RIGHT_EYE,
	eSSP_MONOSCOPIC_EYE
};

struct FMinimalViewInfo
{
	vec3 Location;
	vec3 Rotation;
	float FOV;
	float OrthoWidth;
	float OrthoNearClipPlane;
	float OrthoFarClipPlane;
	float AspectRatio;
	uint32_t bConstrainAspectRatio : 1;
	uint32_t bUseFieldOfViewForLOD : 1;
	byte ProjectionMode;
	float PostProcessBlendWeight;
	//struct FPostProcessSettings PostProcessSettings; // FPostProcessSettings is a huge ass structure so I didn't bother remaking it
	//vec2 OffCenterProjectionOffset;
};

struct UVTabel
{
	inline uintptr_t operator[](const int index) { return items[index]; }

	inline uintptr_t offset(const int offset) { return *(uintptr_t*)((uintptr_t)this + offset); }

	uintptr_t items[256];
};

#define UObject_d \
DEFINE_MEMBER_N0(UVTabel*, VTable);\
DEFINE_MEMBER_N(int, ObjectIndex, 0xC);\
DEFINE_MEMBER_N(void*, Type, 0x10);\
DEFINE_MEMBER_N(int, FNameIndex, 0x18);

struct UObject
{
	union
	{
		UObject_d;
	};
};

struct USceneComponent
{
	union
	{
		UObject_d;
	};
};

struct ACapsuleComponent
{
	union
	{
		UObject_d;
		// ComponentToWorld.Translation (ComponentToWorld @ 0x188, Translation @ +0x10)
		DEFINE_MEMBER_N(vec3, ModifiableLocation, 0x198);
		DEFINE_MEMBER_N(const vec3, RelativeLocation, 0x164);
	};
};

struct ARootComponent
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(FTransform, CompToWorld, 0x188);
		DEFINE_MEMBER_N(vec3, Velocity, 0x1C0);
	};
};

struct ACharacterMovementComponent
{
	enum EMovementMode : byte
	{
		MOVE_None,
		MOVE_Walking,
		MOVE_NavWalking,
		MOVE_Falling,
		MOVE_Swimming,
		MOVE_Flying,
		MOVE_Custom,
		MOVE_MAX
	};

	union
	{
		UObject_d;
		DEFINE_MEMBER_N(float, GravityScale, 0x198);
		DEFINE_MEMBER_N(float, MaxStepHeight, 0x19C);
		DEFINE_MEMBER_N(float, JumpZVelocity, 0x1A0);
		DEFINE_MEMBER_N(EMovementMode, MovementMode, 0x1B0);
		DEFINE_MEMBER_N(byte, CustomMovementMode, 0x1B1);
		DEFINE_MEMBER_N(float, MaxWalkSpeed, 0x1DC);
		DEFINE_MEMBER_N(float, MaxWalkSpeedCrouched, 0x1E0);
		DEFINE_MEMBER_N(float, MaxSwimSpeed, 0x1E4);
		DEFINE_MEMBER_N(float, MaxFlySpeed, 0x1E8);
		DEFINE_MEMBER_N(float, MaxCustomMovementSpeed, 0x1EC);
		DEFINE_MEMBER_N(float, MaxAcceleration, 0x1F0);
		DEFINE_MEMBER_N(float, Mass, 0x258);
		DEFINE_MEMBER_N(byte, bCheatFlying, 0x3D0); // bit 2 (bCheatFlying : 1, BitIndex 0x02)
	};
};

struct PlayerBP_C // local player class
{
	// TMap<FString, UExtraResource_Object_C*> 内存布局（UE 4.16.3）
	// +0x00: Pairs.Data (ptr)         - SetElement 数组
	// +0x08: Pairs.Data.Num (int32)
	// +0x10: AllocationFlags.InlineData (FBitArray)
	// +0x20: AllocationFlags.SecondaryData (ptr, 为空则用 inline)
	// +0x28: AllocationFlags.NumBits (int32)
	struct FExtraResourcesMap
	{
		void* ElementsData;        // 0x00
		int ElementsNum;           // 0x08
		int ElementsMax;           // 0x0C
		uint32_t FlagsInline[4];   // 0x10
		void* FlagsSecondary;      // 0x20
		int FlagsNumBits;          // 0x28
	};

	union
	{
		UObject_d;
		DEFINE_MEMBER_N(byte, bActorEnableCollision, 0x84); // bit 1 (BitIndex 0x01)
		DEFINE_MEMBER_N(struct ARootComponent*, RootComp, 0x158);
		DEFINE_MEMBER_N(struct ACharacterMovementComponent*, CharacterMovement, 0x398);
		DEFINE_MEMBER_N(struct ACapsuleComponent*, CapsuleComponent, 0x3A0);
		DEFINE_MEMBER_N(float, CrosshairExpand, 0x8DC);
		DEFINE_MEMBER_N(float, Health, 0x90C);
		DEFINE_MEMBER_N(byte, Crouched, 0x910);
		DEFINE_MEMBER_N(float, MaxMovementSpeed, 0x914);
		DEFINE_MEMBER_N(byte, Dead, 0x93C);
		DEFINE_MEMBER_N(FExtraResourcesMap, ExtraResources, 0xB30);
		// 当前手持武器/物品（APlayerItem_Parent_C 系：P_Ammo@0x3C8 Infnite_Weapon_Ammo@0x3D0 Infnite_Gadget_Ammo@0x3D1）
		// 游戏区分 Weapon（剑等）与 Item（枪械/飞镖等），切换物品时更新 Current_Player_Item
		DEFINE_MEMBER_N(void*, CurrentPlayerWeapon, 0xE00);
		DEFINE_MEMBER_N(void*, CurrentPlayerItem, 0xE08);
	};

	// 对单个手持物品应用无限弹药（返回是否处理了有效对象）
	__forceinline bool ApplyInfiniteAmmoItem(const void* item, const int ammo) const
	{
		if (!item)
			return false;

		const uintptr_t w = (uintptr_t)item;
		*(bool*)(w + 0x3D0) = true;        // Infnite_Weapon_Ammo
		*(bool*)(w + 0x3D1) = true;        // Infnite_Gadget_Ammo
		*(int*)(w + 0x3C8) = ammo;         // P_Ammo

		// APlayerItem_Parent_C::Extra_Resource_Parent @ 0x370：物品专属弹药资源对象
		// 枪械弹药实际存储于此（UExtraResource_Object_C），非玩家全局 TMap
		void* resParent = *(void**)(w + 0x370);
		if (resParent)
		{
			*(bool*)((uintptr_t)resParent + 0x48) = true; // Infnite_Ammo
			*(int*)((uintptr_t)resParent + 0x30) = ammo;  // Ammo_Int
		}

		return true;
	}

	// 无限弹药：直接打开游戏原生无限弹药开关（APlayerItem_Parent_C::Infnite_Weapon_Ammo /
	// UExtraResource_Object_C::Infnite_Ammo），同时把弹药数补满
	void SetInfiniteAmmo(const int ammo) const
	{
		// 1) 手持武器与物品（两个槽位都处理，覆盖剑/枪/飞镖所有情况）
		ApplyInfiniteAmmoItem(this->CurrentPlayerWeapon, ammo);
		ApplyInfiniteAmmoItem(this->CurrentPlayerItem, ammo);

		// 2) Extra_Resources TMap（投掷物等全局资源）：原生开关 + Ammo_Int 补满
		// SetElement 大小 0x20: TPair{ FString Key(0x10); UExtraResource_Object_C* Value(0x8); } + HashNextId(4) + HashIndex(4)
		const FExtraResourcesMap& map = this->ExtraResources;
		if (!map.ElementsData || map.ElementsNum <= 0)
			return;

		const uintptr_t flagsAddr = map.FlagsSecondary ? (uintptr_t)map.FlagsSecondary : (uintptr_t)&map.FlagsInline;

		for (int i = 0; i < map.ElementsNum && i < map.FlagsNumBits; i++)
		{
			const uint32_t word = *(uint32_t*)(flagsAddr + (i / 32) * sizeof(uint32_t));
			if (!(word & (1u << (i % 32)))) // free slot
				continue;

			const uintptr_t element = (uintptr_t)map.ElementsData + (uintptr_t)i * 0x20;
			void* resObj = *(void**)(element + 0x10); // TPair.Second (Value)
			if (!resObj)
				continue;

			*(bool*)((uintptr_t)resObj + 0x48) = true; // UExtraResource_Object_C::Infnite_Ammo
			*(int*)((uintptr_t)resObj + 0x30) = ammo;  // Ammo_Int
		}
	}

	vec3 GetVelocity() const;
};

struct APlayerState
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(FString, PlayerName, 0x338);
	};
};

struct USkeletalMeshComponent
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(FTransform, CompToWorld, 0x188);
		// UE4.22 私有成员 ComponentSpaceTransformsArray @ 0x5B0：动画评估每帧写入的组件空间骨骼数组
		// 位于 Dumper-7 的 Pad_5B0[0xA8] 区域（MasterPoseComponent@0x5A8 之后）
		// 运行时扫描确认：count=62，头骨四元数模长=1.000，头骨距原点165cm（人体尺度）
		// 注意：0x5C0 是 BoneSpaceTransforms（骨骼空间副本），0x690 区域是 FBoneContainer 的索引数组（非 FTransform）
		DEFINE_MEMBER_N(TArray<FTransform>, AnimTransforms, 0x5B0);
		// UPROPERTY 版 CachedComponentSpaceTransforms：4.22 默认动画路径不填充（恒空），仅作回退
		DEFINE_MEMBER_N(TArray<FTransform>, CachedBoneArray, 0x7A8);
	};

	vec3 GetBonePos(const byte& boneIndex) const
	{
		// 优先动画评估数组（0x5B0），有效则直接用
		if (this->AnimTransforms.items && boneIndex < this->AnimTransforms.count)
		{
			Matrix matrix =
				this->AnimTransforms[boneIndex].ToMatrixWithScale() * this->CompToWorld.ToMatrixWithScale();
			return { matrix.m[3][0], matrix.m[3][1], matrix.m[3][2] };
		}

		// 回退旧版数组（0x7A8），未初始化/越界返回零向量
		if (!this->CachedBoneArray.items || boneIndex >= this->CachedBoneArray.count)
			return {};

		Matrix matrix =
			this->CachedBoneArray[boneIndex].ToMatrixWithScale() * this->CompToWorld.ToMatrixWithScale();

		return { matrix.m[3][0], matrix.m[3][1], matrix.m[3][2] };
	}
};

struct AEnemy : UtilClass // actual class name: EnemyAI_BluePrintV2_C
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(struct ARootComponent*, RootComp, 0x158);
		DEFINE_MEMBER_N(struct USkeletalMeshComponent*, Mesh, 0x390);
		DEFINE_MEMBER_N(float, Health, 0x78C);
		DEFINE_MEMBER_N(vec3, BulletRotation, 0x7B0); // FRotator
		DEFINE_MEMBER_N(float, BulletDamage, 0x7BC);
		DEFINE_MEMBER_N(byte, EnemyState, 0x7E9); // Eenemy_state Enemy_State
		DEFINE_MEMBER_N(byte, bCanSeePlayer, 0x842);
		DEFINE_MEMBER_N(byte, bInRange, 0x843);
		DEFINE_MEMBER_N(byte, CanSee, 0x998);
		DEFINE_MEMBER_N(struct PlayerBP_C*, Player, 0x800);
	};

	// 从 RootComponent 的 ComponentToWorld 四元数提取 yaw（度）
	float GetYaw() const
	{
		if (!this->RootComp) return 0.f;

		const vec4& q = this->RootComp->CompToWorld.Rotation;
		return atan2f(2.f * (q.w * q.y + q.x * q.z), 1.f - 2.f * (q.y * q.y + q.z * q.z)) * (180.f / M_PI);
	}

	__forceinline vec3 GetBonePos(const byte& boneIndex) const
	{
		// 防御：Mesh 为空时（非角色对象/未初始化）返回零向量，避免空指针解引用
		if (!this->Mesh)
			return {};

		return this->Mesh->GetBonePos(boneIndex);
	}

	vec3 GetVelocity() const;

	bool HasValidId() const
	{
		return mem::GetPointerSafe((uintptr_t)this + 0x18, { 0x0 }); // 0x18 = offset of FNameIndex
	}

	bool IsDead() const
	{
		return !this->EnemyState;
	}

	bool IsPlayer() const
	{
		if (!this) return false;
		return
			this->HasValidId() &&																		// is it a invalid pointer
			std::string(mem::GetGName(this->FNameIndex))._Starts_with("EnemyAI_BluePrint") &&			// is enemy type
			this->EnemyState &&																			// is alive
			this->Mesh;																					// has valid skeletal mesh
	}

	bool IsVisible() const;
};

struct APlayerCameraManager
{
	// CameraCachePrivate (FCameraCacheEntry) @ 0x1A20（Dumper-7 Engine_classes.hpp，4.22.3 真实相机缓存）:
	// Timestamp(0x0) + Pad(0xC) + POV(FMinimalViewInfo) @ +0x10
	// FMinimalViewInfo: Location(0x0), Rotation(0xC), FOV(0x18)
	// 注意：旧 CameraCache @ 0x3A0 为废弃 UPROPERTY，游戏从不更新（恒为默认值 Location=0 FOV=90）
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(vec3, Location, 0x1A30);
		DEFINE_MEMBER_N(vec3, Rotation, 0x1A3C);
		DEFINE_MEMBER_N(float, FOV, 0x1A48);
	};

	bool WorldToScreen(const vec3& worldLoc, vec2& screenPos) const
	{
		Matrix tempMatrix = this->Rotation;

		vec3 vDelta = worldLoc - this->Location;

		vec3 vTransformed = {
			vDelta.Dot({ tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2] }),
			vDelta.Dot({ tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2] }),
			vDelta.Dot({ tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2] })
		};

		if (vTransformed.z < 1.f)
			vTransformed.z = 1.f;

		screenPos.x = winCenter.x + vTransformed.x * (winCenter.x / tanf(this->FOV * M_PI / 360)) / vTransformed.z;
		screenPos.y = winCenter.y - vTransformed.y * (winCenter.x / tanf(this->FOV * M_PI / 360)) / vTransformed.z;

		if (screenPos.x > winSize.x || screenPos.x < 0.f || screenPos.y > winSize.y || screenPos.y < 0.f)
			return false;

		return true;
	}
};

struct APlayerController
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(struct USceneComponent*, ActorComponent, 0x378); // AController::TransformComponent
		DEFINE_MEMBER_N(struct PlayerBP_C*, Character, 0x370);           // AController::Character
		DEFINE_MEMBER_N(struct APlayerState*, PlayerState, 0x338);       // AController::PlayerState
		DEFINE_MEMBER_N(vec2, ControlRotation, 0x398);                   // AController::ControlRotation
		DEFINE_MEMBER_N(struct APlayerCameraManager*, CameraManager, 0x3C8); // PlayerCameraManager
	};
};

struct ULocalPlayer
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(struct APlayerController*, PlayerController, 0x30); // UPlayer::PlayerController
		DEFINE_MEMBER_N(struct UGameViewportClient*, ViewportClient, 0x70);
	};
};

struct UGameInstance
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(TArray<struct ULocalPlayer*>, LocalPlayers, 0x38);
	};
};

struct ULevel
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(TArray<struct AEnemy*>, EntityList, 0x98); // Actors
	};
};

struct _UWorld
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(TArray<struct ULevel*>, Levels, 0x138);
		DEFINE_MEMBER_N(struct ULevel*, PersistentLevel, 0x30);
		DEFINE_MEMBER_N(struct UGameInstance*, GameInstance, 0x160); // OwningGameInstance
	};
};

struct UGameViewportClient
{
	union
	{
		UObject_d;
		DEFINE_MEMBER_N(struct _UWorld*, World, 0x78);
		DEFINE_MEMBER_N(struct UGameInstance*, GameInstance, 0x80);
	};
};

inline struct _UWorld* UWorld;
inline struct ULocalPlayer* LocalPlayer;
inline struct TArray<struct AEnemy*>* EntityList;
inline struct APlayerCameraManager* CameraManager;

#undef UObject_d
#undef DEFINE_MEMBER_N
