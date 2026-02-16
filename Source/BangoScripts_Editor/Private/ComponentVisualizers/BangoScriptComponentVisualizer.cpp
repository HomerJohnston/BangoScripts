#include "BangoScriptComponentVisualizer.h"

#include "SceneView.h"
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "Framework/Application/SlateApplication.h"
#include "Utilities/BangoEditorUtility.h"

// ----------------------------------------------

void FBangoScriptComponentVisualizer::OnRegister()
{
	FComponentVisualizer::OnRegister();
}

// ----------------------------------------------

void FBangoScriptComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
}

// ----------------------------------------------

// TODO remove code duplication, try to run Bango::Editor::Draw instead ?
void FBangoScriptComponentVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport,	const FSceneView* View, FCanvas* Canvas)
{
	if (!IsValid(Component) || !View || !Canvas)
	{
		return;
	}
	
	const UBangoScriptComponent* ScriptComponent = Cast<UBangoScriptComponent>(Component);
	if (!ScriptComponent)
	{
		return;
	}
	
	UBangoScriptBlueprint* Blueprint = ScriptComponent->GetScriptBlueprint();
	if (!Blueprint)
	{
		return;
	}
		
	Bango::Editor::DebugDrawActorConnections(*Blueprint, *View, *Canvas);
}

// ----------------------------------------------

bool FBangoScriptComponentVisualizer::GetActorScreenPos(const FSceneView* View, FCanvas* Canvas, const AActor* Actor, FVector& OutWorldPosition, FVector& OutScreenPosition)
{
	FVector TargetBoxExtents;
	Actor->GetActorBounds(false, OutWorldPosition, TargetBoxExtents);
	float TargetSphereRadius = TargetBoxExtents.GetMax() * 0.677f;
	
	FVector2D ScreenPos; 
	if (!GetScreenPos(View, OutWorldPosition, ScreenPos))
	{
		return false;
	}

	FVector RightView = FVector::UpVector.Cross(View->GetViewDirection());
	FVector UpView = RightView.Cross(View->GetViewDirection());
	
	FVector2D RadiusScreenPos;
	if (!GetScreenPos(View, OutWorldPosition + TargetSphereRadius * UpView, RadiusScreenPos))
	{
		return false;
	}
	
	OutScreenPosition.X = ScreenPos.X;
	OutScreenPosition.Y = ScreenPos.Y;
	
	return true;
}

// ----------------------------------------------

bool FBangoScriptComponentVisualizer::GetScreenPos(const FSceneView* View, const FVector& WorldPos, FVector2D& ScreenPos)
{
	FVector4 ScreenPoint = View->WorldToScreen(WorldPos);

	return View->ScreenToPixel(ScreenPoint, ScreenPos);
}

bool FBangoScriptComponentVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	return FComponentVisualizer::VisProxyHandleClick(InViewportClient, VisProxy, Click);
}

// ----------------------------------------------
