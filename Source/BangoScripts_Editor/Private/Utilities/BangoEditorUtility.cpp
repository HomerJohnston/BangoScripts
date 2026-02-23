#include "BangoEditorUtility.h"

#include "ExternalPackageHelper.h"
#include "SceneView.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Core/BangoScriptContainer.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFindActor.h"
#include "BlueprintEditor/BangoScriptBlueprintEditor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "WorldPartition/WorldPartition.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Canvas.h"
#include "Engine/GameViewportClient.h"
#include "Unsorted/BangoActorNodeDraw.h"
#include "Widgets/SViewport.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

// ----------------------------------------------

FString Bango::Editor::GetGameScriptRootFolder()
{
	return TEXT("__BangoScripts__");
}

// ----------------------------------------------

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

Bango::Editor::EBangoScriptType Bango::Editor::GetScriptType(TSubclassOf<UBangoScript> ScriptClass)
{
	if (!ScriptClass)
	{
		return EBangoScriptType::None;
	}

	if (ScriptClass->GetName().StartsWith(Bango::Editor::GetLevelScriptNamePrefix()))
	{
		return EBangoScriptType::LevelScript;
	}

	return EBangoScriptType::ContentAssetScript;
}

// ----------------------------------------------

Bango::Editor::EBangoScriptType Bango::Editor::GetScriptType(TSoftClassPtr<UBangoScript> ScriptClass)
{
	if (ScriptClass.ToSoftObjectPath().ToString().StartsWith("/Game" / Bango::Editor::GetGameScriptRootFolder()))
	{
		return EBangoScriptType::LevelScript;
	}
	
	if (ScriptClass.IsNull())
	{
		return EBangoScriptType::None;
	}
	
	return EBangoScriptType::ContentAssetScript;
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

	TSoftObjectPtr<AActor> OldOwnerActorSoft = SourceBlueprint->GetOwnerActor();
	TSoftObjectPtr<AActor> NewOwnerActorSoft = DuplicateScript->GetOwnerActor();
	
	if (OldOwnerActorSoft != NewOwnerActorSoft)
	{
		FixupBlueprintForNewOwner(DuplicateScript, OldOwnerActorSoft, NewOwnerActorSoft);
	}
	
	return DuplicateScript;
}

// ----------------------------------------------

void Bango::Editor::FixupBlueprintForNewOwner(UBangoScriptBlueprint* ScriptBlueprint, const TSoftObjectPtr<AActor>& OldOwnerActorSoft, const TSoftObjectPtr<AActor>& NewOwnerActorSoft)
{
	check(ScriptBlueprint);
	
	TArray<UEdGraph*> Graphs;
	ScriptBlueprint->GetAllGraphs(Graphs);

	for (const UEdGraph* Graph : Graphs)
	{
		TArray<UK2Node_BangoBase*> AllBangoNodes;
		Graph->GetNodesOfClass<UK2Node_BangoBase>(AllBangoNodes);

		for (UK2Node_BangoBase* Node : AllBangoNodes)
		{
			Node->FixUpForNewOwnerActor(OldOwnerActorSoft, NewOwnerActorSoft);
		}
	}
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
    // Don't try to draw if we don't even have a canvas
	if (!Canvas || !Canvas->Canvas || !Canvas->SceneView)
	{
		return;
	}
    
    if (IsGameViewportFocused())
    {
        return;
    }
    
    // Don't try to draw if we don't have a blueprint
	UBangoScriptBlueprint* Blueprint = ScriptBlueprintEditor->GetBangoScriptBlueprintObj();

	if (!Blueprint)
	{
		return;
	}
	
	DebugDrawActorConnections(*Blueprint, *Canvas->SceneView, *Canvas->Canvas);
}

// ----------------------------------------------

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
	
    if (const UBangoScript* Script = GetDefault<UBangoScript>(ScriptBlueprint.GeneratedClass))
    {
        if (Script->HideActorReferenceIndicators())
        {
            return;
        }
    }

	AActor& OwnerActor = *ScriptBlueprint.GetOwnerActor().Get();

	FName ScriptWPGridName = NAME_None;
	
	auto GetWPGridCellName = [] (TSoftObjectPtr<AActor> ActorSoft) -> FName
	{
		if (AActor* Actor = ActorSoft.Get())
		{
			if (UWorld* World = Actor->GetWorld())
			{
				if (UWorldPartition* WorldPartition = World->GetWorldPartition())
				{
					UActorDescContainerInstance* ActorDescContainer = WorldPartition->GetActorDescContainerInstance();
		
					if (ActorDescContainer)
					{
						const FWorldPartitionActorDescInstance* ActorDesc = ActorDescContainer->GetActorDescInstanceByPath(FSoftObjectPath(Actor));
			
						if (ActorDesc)
						{
							return ActorDesc->GetRuntimeGrid();
						}
					}
				}
			}
		}
		
		return NAME_None;
	};
	
	ScriptWPGridName = GetWPGridCellName(ScriptBlueprint.GetOwnerActor());
	
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
			
				if (ScriptWPGridName != NAME_None)
				{
					FName ThisActorWPGridName = GetWPGridCellName(TargetActor);
					
					UE_LOG(LogTemp, Warning, TEXT("%s --- %s"), *ScriptWPGridName.ToString(), *ThisActorWPGridName.ToString());
				}
				
				if (const AActor* Actor = TargetActor.Get())
				{
					if (Actor->IsLockLocation())
					{
						continue;
					}
					
					FBangoActorNodeDraw DrawRecord;
					DrawRecord.Actor = TargetActor;
					
					bool bAlreadyInSet;
					FBangoActorNodeDraw& Draw = UniqueFindActorNodes.FindOrAddByHash(GetTypeHash(Actor), DrawRecord, &bAlreadyInSet);
					Draw.bFocused = Draw.bFocused || (GFrameCounter - Node->LastSelectedFrame < 3);
				}
			}
			
			const float BaseRadius = 0.005 * View.UnscaledViewRect.Size().Y;

			// Now we draw
			for (const FBangoActorNodeDraw& ActorNode : UniqueFindActorNodes)
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
								
				DrawCircle_ScreenSpace(View, Canvas, TargetActorScreenPos, Radius, Thickness, Color);					

				// TODO: ACTOR STREAMABLE REFS
				/*
				if (TargetActorScreenPos.Z > 0.0f)
				{
					if (const IBangoScriptHolderInterface* Interface = ScriptBlueprint.GetScriptHolder())
					{
						if (Interface->GetScriptContainer().StreamingSourceActorRefs.Contains(ActorNode.Actor))
						{
							FCanvasTextItem StreamingSourceText(FVector2D(TargetActorScreenPos.X, TargetActorScreenPos.Y + 20.0f), INVTEXT("WP STREAM SRC"), GEngine->GetSmallFont(), Bango::Colors::LightOrange);
							StreamingSourceText.bCentreX = true;
							StreamingSourceText.bCentreY = true;
							//StreamingSourceText.bOutlined = true;
							//StreamingSourceText.OutlineColor = Bango::Colors::Noir_Trans;
							
							FCanvasTextItem StreamingSourceText2(StreamingSourceText);
							StreamingSourceText2.Position += FVector2D(1.0f, 1.0f);
							StreamingSourceText2.SetColor(Bango::Colors::Noir);
							
							Canvas.DrawItem(StreamingSourceText2);				
							Canvas.DrawItem(StreamingSourceText);				
						}
					}
				}
				*/
				
				// Draw connection line
				FVector Delta = ActorNode.Actor->GetActorLocation() - DrawOrigin;
				
				const float StartDrawDistance = 30.0f;
				
				if (Delta.SizeSquared() > FMath::Square(StartDrawDistance))
				{
					FVector Start = DrawOrigin;
					FVector End = TargetActorWorldPos;
			
					float DashLength = 0.0f;
					
					if (const IBangoScriptHolderInterface* Interface = ScriptBlueprint.GetScriptHolder())
					{
						if (!Interface->GetScriptContainer().HardActorRefs.Contains(&Actor))
						{
							DashLength = 250.0f;
						}
					}
					
					Bango::Editor::DrawLine_WorldSpace(View, Canvas, Start, End, Thickness, Color, StartDrawDistance, 0.0f, DashLength);
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
	if (ScreenPosition.Z <= 0.0f)
	{
		return;
	}
	
	Canvas.DrawNGon(FVector2D(ScreenPosition.X, ScreenPosition.Y), Color.ToFColor(true), 16, Radius);
}

// ----------------------------------------------

void Bango::Editor::DrawLine_WorldSpace(const FSceneView& View, FCanvas& Canvas, const FVector& WorldStart,	const FVector& WorldEnd, float Thickness, const FLinearColor& Color, float StartCutoff, float EndCutoff, float DashLength)
{
	FVector Delta = WorldEnd - WorldStart;
	if (Delta.SizeSquared() <= FMath::Square(StartCutoff + EndCutoff))
	{
		return;
	}
	
	FVector LineDir = (WorldEnd - WorldStart).GetSafeNormal();
	
	FVector TrueStart = WorldStart + StartCutoff * LineDir;
	FVector TrueEnd = WorldEnd - EndCutoff * LineDir;
	
	FPlane ClipPlane = View.NearClippingPlane;
	
	double StartDistance = FVector::PointPlaneDist(TrueStart, ClipPlane.GetOrigin(), ClipPlane.GetNormal());
	double EndDistance = FVector::PointPlaneDist(TrueEnd, ClipPlane.GetOrigin(), ClipPlane.GetNormal());
	
	if (StartDistance >= 0.0f && EndDistance >= 0.0f)
	{
		return;
	}
	
	if (StartDistance >= 0.0f)
	{
		TrueStart = ClipPlane.GetNormal() + FMath::LinePlaneIntersection(TrueStart, TrueEnd, ClipPlane.GetOrigin(), ClipPlane.GetNormal());
	}

	if (EndDistance >= 0.0f)
	{
		TrueEnd = ClipPlane.GetNormal() + FMath::LinePlaneIntersection(TrueStart, TrueEnd, ClipPlane.GetOrigin(), ClipPlane.GetNormal());
	}
	
	if (DashLength <= KINDA_SMALL_NUMBER)
	{
		DashLength = 999999999.0f;
	}
	
	int32 Segments = 1 + (TrueEnd - TrueStart).Length() / (DashLength);
	
	FRay Ray(TrueStart, TrueEnd - TrueStart);
	
	for (int32 i = 0; i < Segments; ++i)
	{
		FVector DashStart = Ray.Origin + i * (DashLength) * Ray.Direction;

		FVector DashEnd;

		if (i >= Segments - 1)
		{
			DashEnd = TrueEnd;
		}
		else
		{
			DashEnd = DashStart + DashLength * Ray.Direction;
		}

		FVector ScreenStart;
		FVector ScreenEnd;
	
		if (!GetScreenPos(View, DashStart, ScreenStart))
		{
			return;
		}

		if (!GetScreenPos(View, DashEnd, ScreenEnd))
		{
			return;
		}

		if ((DashEnd - DashStart).Dot(LineDir) <= 0.0f)
		{
			return;
		}
	 
		FCanvasLineItem Line(FVector2D(ScreenStart.X, ScreenStart.Y), FVector2D(ScreenEnd.X, ScreenEnd.Y));
	
		FLinearColor DashColor = Color;
		float DashThickness = Thickness;
		
		if (FMath::Modulo(i+1, 2) == 0)
		{
			DashColor = Color * 0.25f;
			DashThickness = Thickness - 1.0f;
		}
		
		if (DashThickness > KINDA_SMALL_NUMBER)
		{
			Line.LineThickness = DashThickness;
			Line.SetColor(DashColor);
			Canvas.DrawItem(Line);
		}
	}
}

// ----------------------------------------------

bool Bango::Editor::GetActorScreenPos(const FSceneView& View, const AActor& Actor, FVector& OutWorldPosition, FVector& OutScreenPosition)
{
	FVector TargetBoxExtents;
	Actor.GetActorBounds(false, OutWorldPosition, TargetBoxExtents);
	
	if (!GetScreenPos(View, OutWorldPosition, OutScreenPosition))
	{
		return false;
	}

	return true;
}

// ----------------------------------------------

bool Bango::Editor::GetScreenPos(const FSceneView& View, const FVector& WorldPos, FVector& ScreenPos)
{
	FVector4 ScreenPoint = View.WorldToScreen(WorldPos);

	FVector2D ScreenPos2D;
	bool bSuccess = View.ScreenToPixel(ScreenPoint, ScreenPos2D);
	
	ScreenPos.X = ScreenPos2D.X;
	ScreenPos.Y = ScreenPos2D.Y;
	ScreenPos.Z = ScreenPoint.W;
	
	return bSuccess;
}

// ----------------------------------------------

bool Bango::Editor::IsGameViewportFocused()
{
    if (GEditor->IsPlaySessionInProgress() && !GEditor->bIsSimulatingInEditor)
    {
        UWorld* World = GEditor->PlayWorld;
		
        if (!World)
        {
            return false;
        }
		
        UGameViewportClient* GameViewportClient = GEditor->GameViewport;
	
        if (!GameViewportClient)
        {
            return false;
        }
		
        TSharedPtr<SViewport> ViewportWidget = GameViewportClient->GetGameViewportWidget();
		
        if (!ViewportWidget.IsValid())
        {
            return false;
        }

        if (ViewportWidget->HasAnyUserFocus())
        {
            return true;
        }
    }
    
	return false;
}

// ----------------------------------------------
