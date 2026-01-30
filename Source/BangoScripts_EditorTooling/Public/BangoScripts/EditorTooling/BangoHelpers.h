#pragma once

#include "CoreTypes.h"
#include "Math/MathFwd.h"
#include "Containers/Map.h"

class AActor;
class UCanvas;
class UActorComponent;

enum class EBangoAllowInvalid : uint8
{
	AllowInvalid,
	RequireValid
};

using enum EBangoAllowInvalid;

#if WITH_EDITOR
namespace Bango::Editor
{	
	BANGOSCRIPTS_EDITORTOOLING_API bool IsComponentInEditedLevel(UObject* Object, EBangoAllowInvalid AllowInvalid = EBangoAllowInvalid::RequireValid);
	
	BANGOSCRIPTS_EDITORTOOLING_API FString UnfixPIEActorPath(const FString& PIEPath);
	
	// BANGOSCRIPTS_EDITORTOOLING_API bool IsComponentBeingDeleted(UActorComponent* Component);
	
	// BANGOSCRIPTS_API UBangoActorIDComponent* GetActorIDComponent(AActor* Actor, bool bForceCreate = false);
	
	// BANGOSCRIPTS_API FName GetBangoName(AActor* Actor);
}
#endif