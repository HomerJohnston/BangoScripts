#pragma once

#include "EditorSubsystem.h"

#include "BangoPIEActorLocationLookupSubsystem.generated.h"

// TODO: ACTOR STREAMABLE REFS
// This is not currently in use and may be removed in the future
UCLASS()
class UBangoPIEActorLocationLookupSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
	
	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	bool ShouldCreateSubsystem(UObject* Outer) const override
	{
		return false;
	}
	
	void BANGOSCRIPTS_EDITORTOOLING_API OnRequestActorLocation(TSoftObjectPtr<AActor> Target, bool& bSuccess, FVector& OutLocation);
};
