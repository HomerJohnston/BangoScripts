#include "BangoDebugDrawService.h"

#include "Editor.h"
#include "SceneView.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "BangoScripts_EditorTooling/BangoScripts_EditorTooling.h"
#include "Debug/DebugDrawService.h"

void UBangoDebugDrawService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UDebugDrawService::Register(TEXT("BangoScriptsShowFlag"), FDebugDrawDelegate::CreateUObject(this, &ThisClass::DebugDraw));
}

void UBangoDebugDrawService::DebugDraw(UCanvas* Canvas, APlayerController* ALWAYSNULL_DONOTUSE)
{
	FBangoDebugDrawCanvas Data(Canvas);
	FBangoEditorDelegates::DebugDrawRequest.Broadcast(Data, GEditor->IsPlayingSessionInEditor());
}

