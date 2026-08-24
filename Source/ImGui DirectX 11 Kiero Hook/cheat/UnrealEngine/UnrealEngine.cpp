#include "UnrealClasses.h"
#include "utils.h"



bool AEnemy::IsVisible() const
{
	vec3 dst = this->GetBonePos(48/*head*/);

	FHitResult hit;
	TraceLine<2>(CameraManager->Location,							// 3d world source
		dst,														// 3d world destination
		hit,														// out hit info
		{ (void*)this, LocalPlayer->PlayerController->Character });	// Actors to ignore											

	if (hit.bBlockingHit)
	{
		AEnemy* actor = hit.GetActor();
		if (actor && std::string(mem::GetGName(actor->FNameIndex))._Starts_with("Glass")) // maybe we hit glass
		{
			if (TraceLine<1>(hit.ImpactPoint, dst, hit, { (void*)actor }, 1)) // pass in 1 as the trace channel because for whatever reason the Glass actor on the 0 trace channel doesn't get ignored although I pass it as an argument to the function
			{
				actor = hit.GetActor();
				if (actor->IsPlayer())
					return true;
			}
		}
		return false;
	}
	return true;
}

AEnemy* FHitResult::GetActor()
{
	// Actor @ 0x68 是 TWeakObjectPtr，其 ObjectIndex 与 UObject::ObjectIndex 同一索引空间，直接匹配
	if (this->Actor.ObjectIndex <= 0)
		return nullptr;

	for (USHORT i = 0; i < EntityList->count; i++)
	{
		AEnemy* ent = EntityList->items[i];
		if (ent && ent->ObjectIndex == this->Actor.ObjectIndex)
			return ent;
	}

	return nullptr;
}

vec3 AEnemy::GetVelocity() const
{
	// 防御：RootComp 为空时返回零向量
	if (!this->RootComp) return {};
	return this->RootComp->Velocity;
}

vec3 PlayerBP_C::GetVelocity() const
{
	if (!this->RootComp) return {};
	return this->RootComp->Velocity;
}
