#include "BangoEditorUtility.h"

#include "ExternalPackageHelper.h"
#include "ObjectTools.h"
#include "SceneView.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFindActor.h"
#include "BlueprintEditor/BangoScriptBlueprintEditor.h"
#include "Components/Viewport.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Canvas.h"
#include "Unsorted/BangoActorNodeDraw.h"

// ----------------------------------------------

struct FBangoActorNodeDraw;

FString Bango::Editor::GetGameScriptRootFolder()
{
	return TEXT("__BangoScripts__");
}

const TCHAR* Bango::Editor::GetLevelScriptNamePrefix()
{
	// We append a funny character to the UObject name to make it invisible in the content browser (this is a hacky hack). Note that a period '.' is not allowed because of some filepath checks in engine code.
	return TEXT("~");
	
	// \U0001F4DC Manuscript / Scroll
	// \U0001F9FE Receipt
	// \U0001F4A5 Explosion
	// \U0001F4CD Pushpin
	// \U0000FE6B Small Form @
	// \U0000FF5E Halfwidth Forms ~
}

// ----------------------------------------------

FString Bango::Editor::GetAbsoluteScriptRootFolder()
{
	return FPaths::ProjectContentDir() / GetGameScriptRootFolder();
}

// ----------------------------------------------

UPackage* Bango::Editor::MakeLevelScriptPackage(UObject* Outer, FGuid Guid)
{
	if (!IsValid(Outer) || Outer->GetFlags() == RF_NoFlags || Outer->HasAnyFlags(RF_BeingRegenerated))
	{
		UE_LOG(LogBango, Error, TEXT("Tried to make a script package but null Outer was passed in!"));
		return nullptr;
	}
	
	UPackage* OuterPackage = Outer->GetPackage();
	
	AActor* Actor = Outer->GetTypedOuter<AActor>();
	
	if (!Actor)
	{
		UE_LOG(LogBango, Error, TEXT("Tried to make a script package for something which does not have an Actor outer! This should never happen."));
		return nullptr;
	}
	
	if (!OuterPackage)
	{
		UE_LOG(LogBango, Error, TEXT("Tried to make a script package for something which does not have a Package! This should never happen."));
		return nullptr;
	}
	
	return MakeLevelScriptPackage_Internal(Actor, OuterPackage, /*InOutBPName, */Guid);
}

// ----------------------------------------------

FString UInt32ToBase36(uint32 Value)
{
	static const TCHAR Alphabet[] = TEXT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	FString Result;
	do
	{
		Result.InsertAt(0, Alphabet[Value % 36]);
		Value /= 36;
	}
	while (Value > 0);

	return Result;
}

// ----------------------------------------------

UPackage* Bango::Editor::MakeLevelScriptPackage_Internal(AActor* Actor, UObject* Outer, FGuid Guid)
{
	check(Actor && Outer && Guid.IsValid());
	
	// The goal here is a file path like this
	// /Game/__BangoScripts__/LevelName/ActorPath/UniqueScriptID.uasset

	FString LevelName = FPackageName::GetShortName(Actor->GetLevel()->GetPackage());
	
	// I am not concerned with collisions; most actors in a game will have one script. Might as well reduce the path length to make it nicer.
	uint32 GuidHash = GetTypeHash(Guid);
	FString GuidHashBase36 = UInt32ToBase36(GuidHash);
	
	FString ActorFolderPath;
	
	UWorld* World = Actor->GetWorld();
	check(World);
	
	if (World->GetWorldPartition())
	{
		// We'll build a subfolder hierarchy that contains the same folders as __ExternalActors__ for world partition. This may make it easier to match up script files to actors e.g. using the revision control window.
		FString ObjectPath = Actor->GetPathName().ToLower();
		FString BaseDir = FExternalPackageHelper::GetExternalObjectsPath(Actor->GetPackage()->GetPathName());
	
		FString ExternalPackageName = FExternalPackageHelper::GetExternalPackageName(Actor->GetPackage()->GetPathName(), ObjectPath);
		ExternalPackageName.RemoveFromStart(BaseDir);
		
		FString ActorClass = Actor->GetClass()->GetFName().ToString();
		ActorFolderPath = ActorClass / ExternalPackageName;
	}
	else
	{
		FString ActorFName = Actor->GetFName().ToString();
		
		ActorFolderPath = ActorFName;
		//ActorFolderPath.RemoveFromStart(ActorClass);
	}
	
	check(!ActorFolderPath.IsEmpty());

	// Generate the preferred final path name
	FString FinalPath = "/Game" / Bango::Editor::GetGameScriptRootFolder() / LevelName / ActorFolderPath / GuidHashBase36;
	
	// Ensure we have a unique pathname
	uint32 Suffix = 0;
	FString OriginalFinalPath = FinalPath;
	while (FPackageName::DoesPackageExist(FinalPath))
	{
		UE_LOG(LogBangoEditor, Warning, TEXT("A level script package already existed, this should not happen! Existing path: %s"), *FinalPath);
		FinalPath = FString::Format(TEXT("{0}_{1}"), { OriginalFinalPath, ++Suffix} );
	}
	
	// Make the package
	UPackage* NewPackage = CreatePackage(*FinalPath);
	NewPackage->SetFlags(RF_Public);
	NewPackage->SetPackageFlags(PKG_NewlyCreated);
	
	FString PackageName = FPackageName::LongPackageNameToFilename(NewPackage->GetName());
	
	return NewPackage;
}

// ----------------------------------------------

UBangoScriptBlueprint* Bango::Editor::MakeLevelScript(UPackage* InPackage, const FString& InName, const FGuid& InScriptGuid)
{
	if (!InPackage)
	{
		return nullptr;
	}
	
	if (InName.IsEmpty())
	{
		return nullptr;
	}
	
	FString AssetName = GetLocalScriptName(InName);
	UBangoScriptBlueprint* ScriptBlueprint = Cast<UBangoScriptBlueprint>(FKismetEditorUtilities::CreateBlueprint(UBangoScript::StaticClass(), InPackage, FName(AssetName), BPTYPE_Normal, UBangoScriptBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass()));
	ScriptBlueprint->SetScriptGuid(InScriptGuid);
	
	FAssetRegistryModule::AssetCreated(ScriptBlueprint);

	FKismetEditorUtilities::CompileBlueprint(ScriptBlueprint);
	
	return ScriptBlueprint;
}

// ----------------------------------------------

UBangoScriptBlueprint* Bango::Editor::DuplicateLevelScript(UBangoScriptBlueprint* SourceBlueprint, UPackage* NewScriptPackage, const FString& InName, const FGuid& NewGuid, AActor* NewOwnerActor)
{
	FString ScriptName = InName;
	
	if (ScriptName.IsEmpty())
	{
		ScriptName = FPackageName::GetShortName(NewScriptPackage);
	}
	
	FString AssetName = GetLocalScriptName(ScriptName);
	
	UBangoScriptBlueprint* DuplicateScript = DuplicateObject(SourceBlueprint, NewScriptPackage, FName(AssetName));
	
	DuplicateScript->Reset();
	
	DuplicateScript->SetScriptGuid(NewGuid);
	DuplicateScript->SetOwnerActor(NewOwnerActor);	
	
	return DuplicateScript;
}

// ----------------------------------------------

FString Bango::Editor::GetLocalScriptName(FString InName)
{
	// We append a funny character to the UObject name to make it invisible in the content browser (this is a hacky hack). Note that a period '.' is not allowed because of some filepath checks in engine code.
	return Bango::Editor::GetLevelScriptNamePrefix() + InName;
}

// ----------------------------------------------

void Bango::Editor::DebugDrawBlueprintToViewport(UCanvas* Canvas, APlayerController* ALWAYS_NULL, FBangoScriptBlueprintEditor* ScriptBlueprintEditor)
{
	if (!Canvas || !Canvas->Canvas || !Canvas->SceneView)
	{
		return;
	}
	
	UBangoScriptBlueprint* Blueprint = ScriptBlueprintEditor->GetBangoScriptBlueprintObj();

	if (!Blueprint)
	{
		return;
	}
	
	DebugDrawActorConnections(*Blueprint, *Canvas->SceneView, *Canvas->Canvas);
	
	/*
	const TSoftObjectPtr<const AActor> OwnerActor = Blueprint->GetOwnerActor();
	const IBangoScriptHolderInterface* ScripHolder = Blueprint->GetScriptHolder(); 

	UEdGraph* FocusedGraph = ScriptBlueprintEditor->GetFocusedGraph();
	
	if (!FocusedGraph)
	{
		return;
	}

	if (!OwnerActor.IsValid())
	{
		return;
	}
	
	const AActor* Actor = OwnerActor.Get();
	
	TOptional<FVector> RepresentingPosition = NullOpt;
	
	if (ScripHolder)
	{
		RepresentingPosition = Actor->GetActorLocation() + ScripHolder->GetDebugDrawOrigin();
	}
	else if (Actor)
	{
		// TODO this should actually never happen, not sure how I should handle it though. 
		RepresentingPosition = Actor->GetActorLocation();
	}
	else
	{
		checkNoEntry();
	}
	
	if (!RepresentingPosition.IsSet())
	{
		return;
	}
		
	TArray<UK2Node_BangoFindActor*> FindActorNodes;
	FocusedGraph->GetNodesOfClass(FindActorNodes);

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
		
			if (Actor)
			{
				FBangoActorNodeDraw DrawRecord;
				DrawRecord.Actor = TargetActor;
				
				bool bAlreadyInSet;
				FBangoActorNodeDraw& Draw = VisitedActors.FindOrAddByHash(GetTypeHash(Actor), DrawRecord, &bAlreadyInSet);
				Draw.bFocused = Draw.bFocused || (GFrameCounter - Node->LastSelectedFrame < 3);
			}
		}
		
		float Radius = 0.005 * Canvas->SizeY;

		// Now we draw
		for (const FBangoActorNodeDraw& ActorNode : VisitedActors)// int32 i = 0; i < VisitedActors FindActorNodes.Num(); ++i)
		{
			if (!ActorNode.Actor.IsValid())
			{
				continue;
			}
				
			const AActor& Actor = *ActorNode.Actor.Get();
			
			float Saturation = ActorNode.bFocused ? 1.0f : 0.8f;
			float Luminosity = ActorNode.bFocused ? 1.0f : 0.8f;
			float Thickness = ActorNode.bFocused ? 3.0f : 1.5f;
			FLinearColor Color = Bango::Colors::Funcs::GetHashedColor(GetTypeHash(ActorNode.Actor), Saturation, Luminosity);
			
			FVector TargetActorWorldPos;
			FVector TargetActorScreenPos;
			
			if (!GetActorScreenPos(Canvas, ActorNode.Actor.Get(), TargetActorWorldPos, TargetActorScreenPos))
			{
				continue;
			}
			
			DrawCircle_ScreenSpace(Canvas, TargetActorScreenPos, Radius, Thickness, Color);		
			
			FVector Start = RepresentingPosition.GetValue();
			FVector End = TargetActorWorldPos;
			const float StartDrawDistance = 50.0f;
			DrawLine_WorldSpace(Canvas, Start, End, Thickness, Color, StartDrawDistance, 0.0f);
		}
	}
	else
	{
		// TODO fast path - just display count stuff
	}
	*/
}

void Bango::Editor::DebugDrawActorConnections(UBangoScriptBlueprint& ScriptBlueprint, const FSceneView& View,	FCanvas& Canvas)
{
	const IBangoScriptHolderInterface* ScriptHolder = ScriptBlueprint.GetScriptHolder();
	
	if (!ScriptHolder)
	{
		return;
	}
	
	if (!ScriptBlueprint.GetOwnerActor().IsValid())
	{
		return;
	}
	
	AActor& OwnerActor = *ScriptBlueprint.GetOwnerActor().Get();
	
	FVector DrawOrigin = OwnerActor.GetActorLocation() + ScriptHolder->GetDebugDrawOrigin();
	
	TArray<UEdGraph*> Graphs;
	ScriptBlueprint.GetAllGraphs(Graphs);

	for (const UEdGraph* Graph : Graphs)
	{
		// TODO can I check for focus?
		
		TArray<UK2Node_BangoFindActor*> AllFindActorNodes;
		Graph->GetNodesOfClass(AllFindActorNodes);

		TSet<FBangoActorNodeDraw> UniqueFindActorNodes;

		// TODO this should be a cvar/ini setting
		const int32 MaxActorVisualizationCount = 100;
		
		// This is kind of arbitrary but if some nutjob builds a script with 1000000 actor nodes I don't want to choke the system to a crawl.
		if (AllFindActorNodes.Num() <= MaxActorVisualizationCount)
		{
			UniqueFindActorNodes.Reserve(AllFindActorNodes.Num());
			
			// First we iterate over the whole list to find all actors to draw
			for (const UK2Node_BangoFindActor* Node : AllFindActorNodes)
			{
				const TSoftObjectPtr<AActor> TargetActor = Node->GetTargetActor();
			
				if (const AActor* Actor = TargetActor.Get())
				{
					FBangoActorNodeDraw DrawRecord;
					DrawRecord.Actor = TargetActor;
					
					bool bAlreadyInSet;
					FBangoActorNodeDraw& Draw = UniqueFindActorNodes.FindOrAddByHash(GetTypeHash(Actor), DrawRecord, &bAlreadyInSet);
					Draw.bFocused = Draw.bFocused || (GFrameCounter - Node->LastSelectedFrame < 3);
				}
			}
			
			const float BaseRadius = 0.005 * View.UnscaledViewRect.Size().Y;// Canvas.GetViewRect().Size().Y;// Viewport.GetSizeXY().Y;

			// Now we draw
			for (const FBangoActorNodeDraw& ActorNode : UniqueFindActorNodes)// int32 i = 0; i < VisitedActors FindActorNodes.Num(); ++i)
			{
				if (!ActorNode.Actor.IsValid())
				{
					continue;
				}
				
				const AActor& Actor = *ActorNode.Actor.Get();
				
				float Saturation = ActorNode.bFocused ? 1.0f : 0.6f;
				float Luminosity = ActorNode.bFocused ? 1.0f : 0.6f;
				float Thickness = ActorNode.bFocused ? 3.0f : 1.0f;
				float Radius = ActorNode.bFocused ? 2.0f * BaseRadius : BaseRadius; 
				FLinearColor Color = Bango::Colors::Funcs::GetHashedColor(GetTypeHash(ActorNode.Actor), Saturation, Luminosity);
				
				// Draw circle
				FVector TargetActorWorldPos;
				FVector TargetActorScreenPos;
				if (!GetActorScreenPos(View, Actor, TargetActorWorldPos, TargetActorScreenPos))
				{
					return;
				}
				
				Bango::Editor::DrawCircle_ScreenSpace(View, Canvas, TargetActorScreenPos, Radius, Thickness, Color);					
				
				// Draw connection line
				FVector Delta = ActorNode.Actor->GetActorLocation() - DrawOrigin;
				
				const float StartDrawDistance = 30.0f;
				
				if (Delta.SizeSquared() > FMath::Square(StartDrawDistance))
				{
					FVector Start = DrawOrigin;
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
}

// ----------------------------------------------

void Bango::Editor::DrawCircle_ScreenSpace(UCanvas& Canvas, const FVector& ScreenPosition, float Radius, float Thickness, const FLinearColor& Color)
{
	FSceneView* View = Canvas.SceneView;
	
	if (!View || !Canvas.Canvas)
	{
		return;
	}
	
	DrawCircle_ScreenSpace(*View, *Canvas.Canvas, ScreenPosition, Radius, Thickness, Color);
}

// ----------------------------------------------

void Bango::Editor::DrawLine_WorldSpace(UCanvas& Canvas, const FVector& WorldStart, const FVector& WorldEnd, float Thickness, const FLinearColor& Color, float StartCutoff, float EndCutoff)
{
	FSceneView* View = Canvas.SceneView;
	
	if (!View || !Canvas.Canvas)
	{
		return;
	}
	
	DrawLine_WorldSpace(*View, *Canvas.Canvas, WorldStart, WorldEnd, Thickness, Color, StartCutoff, EndCutoff);
}

// ----------------------------------------------

bool Bango::Editor::GetActorScreenPos(const UCanvas& Canvas, const AActor& Actor, FVector& OutWorldPosition, FVector& OutScreenPosition)
{
	const FSceneView* View = Canvas.SceneView;
	
	if (!View)
	{
		return false;
	}

	return GetActorScreenPos(*View, Actor, OutWorldPosition, OutScreenPosition);
}

// ----------------------------------------------

void Bango::Editor::DrawCircle_ScreenSpace(const FSceneView& View, FCanvas& Canvas, const FVector& ScreenPosition, float Radius, float Thickness, const FLinearColor& Color)
{
	//uint8 NumLineSegments = 24;
	//static TArray<FVector2D> TempPoints;
	//TempPoints.Empty(NumLineSegments);

	/*
	const float NumFloatLineSegments = (float)NumLineSegments;
	for (uint8 i = 0; i <= NumLineSegments; ++i)
	{
		const float Angle = (i / NumFloatLineSegments) * TWO_PI;

		FVector2D PointOnCircle;
		PointOnCircle.X = cosf(Angle) * Radius + ScreenPosition.X;
		PointOnCircle.Y = sinf(Angle) * Radius + ScreenPosition.Y;
		TempPoints.Add(PointOnCircle);
	}
	*/
	
	//for (uint8 i = 0; i <= NumLineSegments; ++i)
	//{
		Canvas.DrawNGon(FVector2D(ScreenPosition.X, ScreenPosition.Y), Color.ToFColor(true), 16, Radius);
	//}
}

// ----------------------------------------------

void Bango::Editor::DrawLine_WorldSpace(const FSceneView& View, FCanvas& Canvas, const FVector& WorldStart,	const FVector& WorldEnd, float Thickness, const FLinearColor& Color, float StartCutoff, float EndCutoff)
{
	FVector2D ScreenStart;
	FVector2D ScreenEnd;
	
	FVector Delta = WorldEnd - WorldStart;
	if (Delta.SizeSquared() <= FMath::Square(StartCutoff + EndCutoff))
	{
		return;
	}
	
	FVector LineDir = (WorldEnd - WorldStart).GetSafeNormal();
	
	FVector TrueStart = WorldStart + StartCutoff * LineDir;
	FVector TrueEnd = WorldEnd - EndCutoff * LineDir;
	
	if (!GetScreenPos(View, TrueStart, ScreenStart))
	{
		return;
	}
	
	if (!GetScreenPos(View, TrueEnd, ScreenEnd))
	{
		return;
	}
	
	if ((TrueEnd - TrueStart).Dot(LineDir) <= 0.0f)
	{
		return;
	}
	
	FCanvasLineItem Line(ScreenStart, ScreenEnd);
	Line.LineThickness = Thickness;
	Line.SetColor(Color);
	Canvas.DrawItem(Line);
}

// ----------------------------------------------

bool Bango::Editor::GetActorScreenPos(const FSceneView& View, const AActor& Actor, FVector& OutWorldPosition, FVector& OutScreenPosition)
{
	FVector TargetBoxExtents;
	Actor.GetActorBounds(false, OutWorldPosition, TargetBoxExtents);
	float TargetSphereRadius = TargetBoxExtents.GetMax() * 0.677f;
	
	FVector2D ScreenPos; 
	if (!GetScreenPos(View, OutWorldPosition, ScreenPos))
	{
		return false;
	}

	FVector RightView = FVector::UpVector.Cross(View.GetViewDirection());
	FVector UpView = RightView.Cross(View.GetViewDirection());
	
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

bool Bango::Editor::GetScreenPos(const FSceneView& View, const FVector& WorldPos, FVector2D& ScreenPos)
{
	FVector4 ScreenPoint = View.WorldToScreen(WorldPos);

	return View.ScreenToPixel(ScreenPoint, ScreenPos);
}
