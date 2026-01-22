#pragma once

#include "ComponentVisualizer.h"

class FBangoScriptComponentVisualizer : public FComponentVisualizer
{
	
	void OnRegister() override;
	
	void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	
	void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	
	void DrawCircle_ScreenSpace(const FSceneView* View, FCanvas* Canvas, const FVector& ScreenPosition, float Radius, float Thickness, const FLinearColor& Color);
	
	void DrawLine_WorldSpace(const FSceneView* View, FCanvas* Canvas, const FVector& WorldStart, const FVector& WorldEnd, float Thickness, const FLinearColor& Color, float StartCutoff = 0.0f, float EndCutoff = 0.0f);
	
	bool GetActorScreenPos(const FSceneView* View, FCanvas* Canvas, const AActor* Actor, FVector& OutWorldPosition, FVector& OutScreenPosition);
	
	bool GetScreenPos(const FSceneView* View, const FVector& WorldPos, FVector2D& ScreenPos);
};
