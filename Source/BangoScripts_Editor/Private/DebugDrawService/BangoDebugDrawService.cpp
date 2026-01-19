#include "BangoDebugDrawService.h"

#include "Editor.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "Debug/DebugDrawService.h"

void UBangoDebugDrawService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UDebugDrawService::Register(TEXT("BangoScriptsShowFlag"), FDebugDrawDelegate::CreateUObject(this, &ThisClass::DebugDraw));
}

void UBangoDebugDrawService::DebugDraw(UCanvas* Canvas, APlayerController* ALWAYSNULL_DONOTUSE) const
{
	FBangoDebugDrawCanvas Data(Canvas);
	
	FBangoEditorDelegates::BangoDebugDraw.Broadcast(Data, GEditor->IsPlayingSessionInEditor());
}
