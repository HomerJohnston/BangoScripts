

#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"

#include "LevelEditor.h"
#include "SLevelViewport.h"
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

FVector FBangoDebugDrawCanvas::GetNextScreenPos(AActor* Actor)
{
	FCanvasDrawLocation& DrawLocation = DrawLocations.FindOrAdd(Actor);
	
	if (DrawLocation.LineIndex == 0)
	{
		FVector ScreenLocation;
		if (GetScreenLocationAboveActor(Actor, ScreenLocation))
		{
			DrawLocation.InitialPositon = ScreenLocation;
		}
	}

	return DrawLocation.InitialPositon + FVector(0.0f, GetLineHeight() * DrawLocation.LineIndex++, 0.0f);
}

float FBangoDebugDrawCanvas::GetAlpha(float Distance) const
{
	float MinDistanceSq = FMath::Square(MinDrawDistance);
	float MaxDistanceSq = FMath::Square(MaxDrawDistance);
	float DistanceSq = Distance * Distance;
	
	// This isn't linear but I'm OK with that for debug drawing
	float LerpAlpha = FMath::Clamp((DistanceSq - MinDistanceSq) / (MaxDistanceSq - MinDistanceSq), 0.0f, 1.0f);
	
	return 1.0f - LerpAlpha;
}

// ----------------------------------------------

float FBangoDebugDrawCanvas::GetAlpha(const FVector& WorldLocation) const
{
	FVector CameraPos;
	GetCameraPos(CameraPos);
	
	float MinDistanceSq = FMath::Square(MinDrawDistance);
	float MaxDistanceSq = FMath::Square(MaxDrawDistance);
	float DistanceSq = FVector::DistSquared(CameraPos, WorldLocation);
	
	// This isn't linear but I'm OK with that for debug drawing
	float LerpAlpha = FMath::Clamp((DistanceSq - MinDistanceSq) / (MaxDistanceSq - MinDistanceSq), 0.0f, 1.0f);
	
	return 1.0f - LerpAlpha;
}

// ----------------------------------------------

float FBangoDebugDrawCanvas::GetLineHeight() const
{
	return 32.0f;
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

bool FBangoDebugDrawCanvas::GetScreenLocationAboveActor(const AActor* Actor, FVector& ScreenLocation) const
{
	check(Canvas);
	check(Actor);
	
	FVector TargetOrigin;
	FVector TargetBoxExtents;
	Actor->GetActorBounds(false, TargetOrigin, TargetBoxExtents);
	float ActorHeight = TargetBoxExtents.Z * 0.5f + HeightAboveActor;
	
	ScreenLocation = Canvas->Project(Actor->GetActorLocation() + FVector(0.0f, 0.0f, ActorHeight), false);
	
	if (ScreenLocation.Z < 0.0f)
	{
		return false;
	}
	
	return true;
}

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

