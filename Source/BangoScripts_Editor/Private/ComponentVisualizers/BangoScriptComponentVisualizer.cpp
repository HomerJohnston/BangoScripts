#include "BangoScriptComponentVisualizer.h"

#include "LevelEditor.h"
#include "SceneView.h"
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFindActor.h"
#include "BangoScripts/EditorTooling/BangoEditorUtility.h"
#include "Components/BillboardComponent.h"
#include "Components/Viewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Unsorted/BangoActorNodeDraw.h"
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
	
	/*
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	
	FVector ComponentActorWorldPos;
	FVector ComponentActorScreenPos;
	if (!GetActorScreenPos(View, Canvas, ComponentActor, ComponentActorWorldPos, ComponentActorScreenPos))
	{
		return;
	}
				
	for (const UEdGraph* Graph : Graphs)
	{
		TArray<UK2Node_BangoFindActor*> FindActorNodes;
		Graph->GetNodesOfClass(FindActorNodes);

		TSet<FBangoActorNodeDraw> VisitedActors;

		// TODO this should be a cvar/ini setting
		const int32 MaxActorVisualizationCount = 100;
		
		// This is kind of arbitrary but if some nutjob builds a script with 1000000 actor nodes I don't want to choke the system to a crawl.
		if (FindActorNodes.Num() <= MaxActorVisualizationCount)
		{
			VisitedActors.Reserve(FindActorNodes.Num());
			
			// First we iterate over the whole list to find all actors to draw
			for (const UK2Node_BangoFindActor* Node : FindActorNodes)
			{
				const TSoftObjectPtr<AActor> TargetActor = Node->GetTargetActor();
			
				if (const AActor* Actor = TargetActor.Get())
				{
					FBangoActorNodeDraw DrawRecord;
					DrawRecord.Actor = TargetActor;
					
					bool bAlreadyInSet;
					FBangoActorNodeDraw& Draw = VisitedActors.FindOrAddByHash(GetTypeHash(Actor), DrawRecord, &bAlreadyInSet);
					Draw.bFocused = Draw.bFocused || (GFrameCounter - Node->LastSelectedFrame < 3);
				}
			}
			
			float Radius = 0.005 * Viewport->GetSizeXY().Y;

			// Now we draw
			for (const FBangoActorNodeDraw& DrawInfo : VisitedActors)// int32 i = 0; i < VisitedActors FindActorNodes.Num(); ++i)
			{
				float Saturation = DrawInfo.bFocused ? 1.0f : 0.8f;
				float Luminosity = DrawInfo.bFocused ? 1.0f : 0.8f;
				float Thickness = DrawInfo.bFocused ? 3.0f : 1.5f;
				FLinearColor Color = Bango::Colors::Funcs::GetHashedColor(GetTypeHash(DrawInfo.Actor), Saturation, Luminosity);
				
				// Draw circle
				FVector TargetActorWorldPos;
				FVector TargetActorScreenPos;
				if (!GetActorScreenPos(View, Canvas, DrawInfo.Actor.Get(), TargetActorWorldPos, TargetActorScreenPos))
				{
					return;
				}
				
				Bango::Editor::DrawCircle_ScreenSpace(View, Canvas, TargetActorScreenPos, Radius, Thickness, Color);					
				
				// Draw connection line
				FVector Delta = DrawInfo.Actor->GetActorLocation() - ScriptComponent->GetBillboard()->GetComponentLocation();
				
				const float StartDrawDistance = 50.0f;
				
				if (Delta.SizeSquared() > FMath::Square(StartDrawDistance))
				{
					FVector Start = ScriptComponent->GetBillboard()->GetComponentLocation();
					FVector End = TargetActorWorldPos;
			
					Bango::Editor::DrawLine_WorldSpace(View, Canvas, Start, End, Thickness, Color, StartDrawDistance);
				}
			}
		}
		else
		{
			// TODO fast path - just display count stuff
		}
	}
	*/
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
