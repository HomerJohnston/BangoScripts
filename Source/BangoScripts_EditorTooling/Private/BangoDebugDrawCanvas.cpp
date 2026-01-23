

#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"

#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "BangoScripts/EditorTooling/BangoDevSettings.h"
#include "Engine/Canvas.h"
#include "GameFramework/Actor.h"
#include "Math/Vector.h"
#include "Modules/ModuleManager.h"
#include "Slate/SceneViewport.h"

// ----------------------------------------------

/*
float FBangoDebugDrawCanvas::GetNextYOffset(AActor* Actor)
{
	uint32& Index = DrawIndices.FindOrAdd(Actor);
	return Index++ * GetYPadding();
}
*/

// ----------------------------------------------

// ----------------------------------------------

FBangoDebugDrawCanvas::FBangoDebugDrawCanvas(UCanvas* InCanvas): Canvas(InCanvas)
{
	
}

// ----------------------------------------------

float FBangoDebugDrawCanvas::GetAlpha(const FVector& WorldLocation, bool bPIE) const
{
	FVector CameraPos;
	GetCameraPos(CameraPos);
	
    float MaxDistance = UBangoScriptsDeveloperSettings::GetPIEDisplayDistance();
    float FadeStartDistance = 0.9f * MaxDistance;
    
	float MinDistanceSq = FMath::Square(FadeStartDistance);
	float MaxDistanceSq = FMath::Square(MaxDistance);
	float DistanceSq = FVector::DistSquared(CameraPos, WorldLocation);
	
	// This isn't linear but I'm OK with that for debug drawing
	float LerpAlpha = FMath::Clamp((DistanceSq - MinDistanceSq) / (MaxDistanceSq - MinDistanceSq), 0.0f, 1.0f);
	
	float Alpha = 1.0f - LerpAlpha;
	
    // Clamp the alpha so it always shows, just really dim if you're far away
	Alpha = FMath::Clamp(Alpha, 0.25f, 1.0f);
	
	return Alpha;
}

// ----------------------------------------------

void FBangoDebugDrawCanvas::GetCameraPos(FVector& CameraPos) const
{
	check(Canvas);

	FVector DummyDir;
	GetCameraPos(CameraPos, DummyDir);
}

// ----------------------------------------------

void FBangoDebugDrawCanvas::GetCameraPos(FVector& CameraPos, FVector& CameraDir) const
{
	check(Canvas);
	
	double X, Y;
	Canvas->GetCenter(X, Y);
	Canvas->Deproject(FVector2D(X, Y), CameraPos, CameraDir);
}

// ----------------------------------------------

bool FBangoDebugDrawCanvas::GetScreenLocation(const FVector& WorldPos, FVector& OutScreenPos)
{
	OutScreenPos = Canvas->Project(WorldPos, false);
	
	return OutScreenPos.Z > 0;
}

// ----------------------------------------------

bool FBangoDebugDrawCanvas::GetScreenLocation(const USceneComponent* SceneComponent, FVector& OutScreenPos)
{
	return GetScreenLocation(SceneComponent->GetComponentLocation(), OutScreenPos);
}

// ----------------------------------------------

bool FBangoDebugDrawCanvas::GetScreenLocation(const AActor* Actor, FVector& OutScreenPos)
{
	return GetScreenLocation(Actor->GetActorLocation(), OutScreenPos);
}

// ----------------------------------------------

bool FBangoDebugDrawCanvas::GetMousePosInLevelViewport(FIntPoint& OutMousePos) const
{
	TSharedPtr<SLevelViewport> LevelViewport = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetFirstActiveLevelViewport();
	TSharedPtr<FSceneViewport> SceneViewport = nullptr;
	
	if (LevelViewport.IsValid())
	{
		SceneViewport = LevelViewport->GetSceneViewport();
	}
	
	if (!SceneViewport)
	{
		return false;
	}
	
	SceneViewport->GetMousePos(OutMousePos);
	
	return true;
}

// ----------------------------------------------

