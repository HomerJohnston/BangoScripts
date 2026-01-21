
#pragma once

#include "Math/Vector.h"
#include "Containers/Map.h"

class AActor;
class UCanvas;
class USceneComponent;

struct BANGOSCRIPTS_EDITORTOOLING_API FBangoDebugDrawCanvas
{
	struct FCanvasDrawLocation
	{
		FVector InitialPositon = FVector::BackwardVector;
		uint32 LineIndex = 0;
	};
	
	friend class UBangoDebugDrawService;
	
	// CONSTRUCTOR
private:
	FBangoDebugDrawCanvas(UCanvas* InCanvas);;
	
	// PUBLIC API
public:
	// Access the UCanvas to draw on
	UCanvas* operator->() { return Canvas; } 
	
	float GetAlpha(const FVector& WorldLocation, bool bPIE) const;
	
	// STATE
private:
	// The canvas we're drawing on, access via -> overload
	UCanvas* Canvas = nullptr;
	
	// 
	TMap<AActor*, FCanvasDrawLocation> DrawLocations;
	
	// INTERNAL
private:

	// Gets camera position in worldspace
	void GetCameraPos(FVector& CameraPos) const;
	
	// Gets camera position in worldspace
	void GetCameraPos(FVector& CameraPos, FVector& CameraDir) const;

public:
	bool GetScreenLocation(const FVector& WorldPos, FVector& OutScreenPos);
	
	bool GetScreenLocation(const USceneComponent* SceneComponent, FVector& OutScreenPos);
	
	bool GetScreenLocation(const AActor* Actor, FVector& OutScreenPos);
	
	bool GetMousePosInLevelViewport(FIntPoint& OutMousePos) const;
};
