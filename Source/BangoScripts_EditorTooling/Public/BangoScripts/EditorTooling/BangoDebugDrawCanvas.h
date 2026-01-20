
#pragma once

#include "Math/Vector.h"
#include "Containers/Map.h"

class AActor;
class UCanvas;

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
	
	// SETTINGS
private:
	float MinDrawDistance = 4000.0f; // TODO cvar/project settings?
	float MaxDrawDistance = 5000.0f; // TODO cvar/project settings?
	float HeightAboveActor = 50.0f; // TODO cvar/project settings?
	
	// PUBLIC API
public:
	// Access the UCanvas to draw on
	UCanvas* operator->() { return Canvas; } 

	// Gets the top-middle screen position to start drawing in
	FVector GetNextScreenPos(AActor* Actor);
		
	// Gets an alpha value based on Min/Max Draw Distance settings
	float GetAlpha(AActor* Actor) const;
	
	// STATE
private:
	// The canvas we're drawing on, access via -> overload
	UCanvas* Canvas = nullptr;
	
	// 
	TMap<AActor*, FCanvasDrawLocation> DrawLocations;
	
	// INTERNAL
private:

	// The nominal vertical gap between users
	float GetLineHeight() const; 
	
	// Gets camera position in worldspace
	void GetCameraPos(FVector& CameraPos) const;
	
	// Gets camera position in worldspace
	void GetCameraPos(FVector& CameraPos, FVector& CameraDir) const;
	
	
	// Gets actor position in screen space, returns false if the actor is not visible
	bool GetScreenLocationAboveActor(AActor* Actor, FVector& ScreenLocation) const; 
};
