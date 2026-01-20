

#pragma once

#include "EditorSubsystem.h"
#include "TickableEditorObject.h"
#include "Engine/Canvas.h"

#include "BangoDebugDrawService.generated.h"

UCLASS()
class UBangoDebugDrawService : public UEditorSubsystem
{
	GENERATED_BODY()

	bool bShowFlagEnabled;

	uint64 DebugDrawFrame;
	
	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void DebugDraw(UCanvas* Canvas, APlayerController* ALWAYSNULL_DONOTUSE);
};
