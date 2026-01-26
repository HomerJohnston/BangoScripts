// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/StrongObjectPtrTemplates.h"

class IBangoScriptHolderInterface;
class UActorComponent;
class UTexture2D;

struct FBangoScriptBillboards
{
	// Key is a texture asset. Value is a generated texture including the default script billboard icon and the supplied texture asset overlaid on top. 
	BANGOSCRIPTS_EDITORTOOLING_API static TMap<TSoftObjectPtr<UTexture2D>, TStrongObjectPtr<UTexture2D>> GeneratedBillboards;
};

namespace Bango
{
	namespace Debug
	{
		BANGOSCRIPTS_EDITORTOOLING_API const TCHAR* ScriptsShowFlagName();
		
	    BANGOSCRIPTS_EDITORTOOLING_API UTexture2D* GetScriptPIESprite();
	    
	    BANGOSCRIPTS_EDITORTOOLING_API UTexture2D* GetScriptBillboardSprite(IBangoScriptHolderInterface* Requester, TSoftObjectPtr<UTexture2D> OverlaySoft = nullptr);
    
	    BANGOSCRIPTS_EDITORTOOLING_API UTexture2D* GetDefaultScriptBillboardSprite();

		BANGOSCRIPTS_EDITORTOOLING_API void LoadIcon(TStrongObjectPtr<UTexture2D>& Destination, const FString& Path);
	    
		BANGOSCRIPTS_EDITORTOOLING_API void PrintComponentState(UActorComponent* Component, FString Msg);

		BANGOSCRIPTS_EDITORTOOLING_API FString GetFlagsString(UObject* Object);
		
		namespace Draw
		{
			BANGOSCRIPTS_EDITORTOOLING_API void DebugDrawDashedLine(UWorld* World, const FVector& Start, const FVector& End, float DashLength, const FColor& Color, bool bPersistentLines = false, float Lifetime = 0, uint8 DepthPriority = 0, float Thickness = 0.0f);
		
			BANGOSCRIPTS_EDITORTOOLING_API void DebugDrawDashedLine(UWorld* World, const FVector& Start, const FVector& End, int32 NumDashes, const FColor& Color, bool bPersistentLines = false, float Lifetime = 0, uint8 DepthPriority = 0, float Thickness = 0.0f);

			BANGOSCRIPTS_EDITORTOOLING_API void DebugDrawDashedLine(UWorld* World, const FRay& Ray, float Distance, float DashLength, const FColor& Color, bool bPersistentLines = false, float Lifetime = 0, uint8 DepthPriority = 0, float Thickness = 0.0f);
		}
	}
}
