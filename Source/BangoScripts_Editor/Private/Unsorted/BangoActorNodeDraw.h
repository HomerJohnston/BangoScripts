#pragma once

#include "UObject/SoftObjectPtr.h"
class AActor;

struct FBangoActorNodeDraw
{
	TSoftObjectPtr<const AActor> Actor = nullptr;
	bool bFocused = false;
	
	bool operator==(const FBangoActorNodeDraw& Other) const { return Other.Actor == this->Actor; }
	
	friend uint32 GetTypeHash(const FBangoActorNodeDraw& Struct)
	{
		return GetTypeHash(Struct.Actor);
	}
};
