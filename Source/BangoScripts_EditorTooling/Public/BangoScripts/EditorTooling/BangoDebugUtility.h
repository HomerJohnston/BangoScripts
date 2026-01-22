// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#pragma once

#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "Containers/UnrealString.h"
#include "UObject/ObjectMacros.h"
#include "UObject/StrongObjectPtrTemplates.h"

class UActorComponent;
class UTexture2D;

namespace Bango
{
	namespace Debug
	{
	    BANGOSCRIPTS_EDITORTOOLING_API UTexture2D* GetScriptPIESprite();
	    
	    BANGOSCRIPTS_EDITORTOOLING_API UTexture2D* GetScriptBillboardSprite();
    
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
