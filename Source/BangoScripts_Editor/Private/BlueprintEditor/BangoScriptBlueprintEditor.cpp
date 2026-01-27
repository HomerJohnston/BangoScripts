#include "BangoScriptBlueprintEditor.h"

#include "EdGraphSchema_K2_Actions.h"
#include "K2Node_Literal.h"
#include "SBlueprintEditorToolbar.h"
#include "Application/ThrottleManager.h"
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoDevSettings.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Private/BangoEditorStyle.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFindActor.h"
#include "Debug/DebugDrawService.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Internationalization/Regex.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/DebuggerCommands.h"
#include "Utilities/BangoEditorUtility.h"
#include "WorldPartition/ActorDescContainerInstance.h"
#include "WorldPartition/WorldPartition.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ----------------------------------------------

FBangoScriptBlueprintEditor::FBangoScriptBlueprintEditor() : FBlueprintEditor()
{
	FBangoEditorDelegates::OnBangoScriptRan.AddRaw(this, &FBangoScriptBlueprintEditor::OnBangoScriptRan);
	FBangoEditorDelegates::OnBangoScriptFinished.AddRaw(this, &FBangoScriptBlueprintEditor::OnBangoScriptFinished);

	DebugDrawHandle = UDebugDrawService::Register(Bango::Debug::ScriptsShowFlagName(), FDebugDrawDelegate::CreateStatic(Bango::Editor::DebugDrawBlueprintToViewport, this));
	
	if (!UBangoScriptsDeveloperSettings::GetPreventSlateThrottlingOverrides())
	{
		FSlateThrottleManager::Get().DisableThrottle(true);
		bSlateThrottlingDisabled = true;	
	}
}

// ----------------------------------------------

FBangoScriptBlueprintEditor::~FBangoScriptBlueprintEditor()
{
	FBangoEditorDelegates::OnBangoScriptRan.RemoveAll(this);
	FBangoEditorDelegates::OnBangoScriptFinished.RemoveAll(this);
	
	if (DebugDrawHandle.IsValid())
	{
		UDebugDrawService::Unregister(DebugDrawHandle);
	}
	
	if (bSlateThrottlingDisabled)
	{
		FSlateThrottleManager::Get().DisableThrottle(false);
	}
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::SetupGraphEditorEvents(UEdGraph* InGraph, SGraphEditor::FGraphEditorEvents& InEvents)
{
	FBlueprintEditor::SetupGraphEditorEvents(InGraph, InEvents);
	InEvents.OnDropActors.Unbind();
	InEvents.OnDropActors = SGraphEditor::FOnDropActors::CreateSP(this, &FBangoScriptBlueprintEditor::OnDropActors);
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::SetupGraphEditorEvents_Impl(UBlueprint* Blueprint, UEdGraph* InGraph, SGraphEditor::FGraphEditorEvents& InEvents)
{
	SetupGraphEditorEvents(InGraph, InEvents);
		
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP( this, &FBangoScriptBlueprintEditor::OnSelectedNodesChanged );
	InEvents.OnDropActors = SGraphEditor::FOnDropActors::CreateSP( this, &FBangoScriptBlueprintEditor::OnGraphEditorDropActor );
	InEvents.OnDropStreamingLevels = SGraphEditor::FOnDropStreamingLevels::CreateSP( this, &FBangoScriptBlueprintEditor::OnGraphEditorDropStreamingLevel );
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FBangoScriptBlueprintEditor::OnNodeDoubleClicked);
	InEvents.OnVerifyTextCommit = FOnNodeVerifyTextCommit::CreateSP(this, &FBangoScriptBlueprintEditor::OnNodeVerifyTitleCommit);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FBangoScriptBlueprintEditor::OnNodeTitleCommitted);
	InEvents.OnSpawnNodeByShortcutAtLocation = SGraphEditor::FOnSpawnNodeByShortcutAtLocation::CreateSP(this, &FBangoScriptBlueprintEditor::OnSpawnGraphNodeByShortcut, InGraph);
	InEvents.OnNodeSpawnedByKeymap = SGraphEditor::FOnNodeSpawnedByKeymap::CreateSP(this, &FBangoScriptBlueprintEditor::OnNodeSpawnedByKeymap );
	//InEvents.OnDisallowedPinConnection = SGraphEditor::FOnDisallowedPinConnection::CreateSP(this, &FBangoBlueprintEditor::OnDisallowedPinConnection);
	InEvents.OnDoubleClicked = SGraphEditor::FOnDoubleClicked::CreateSP(this, &FBangoScriptBlueprintEditor::NavigateToParentGraphByDoubleClick);
		
	// Custom menu for K2 schemas
	if(InGraph->Schema != nullptr && InGraph->Schema->IsChildOf(UEdGraphSchema_K2::StaticClass()))
	{
		InEvents.OnCreateActionMenuAtLocation = SGraphEditor::FOnCreateActionMenuAtLocation::CreateSP(this, &FBangoScriptBlueprintEditor::OnCreateGraphActionMenu);
	}
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::Tick(float DeltaTime)
{
	if (bRequestedSavingOpenDocumentState)
	{
		bRequestedSavingOpenDocumentState = false;

		SaveEditedObjectState();
	}

	OpenTime += DeltaTime;

	if (bPendingDeferredClose)
	{
		IAssetEditorInstance* EditorInst = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(GetBlueprintObj(), /*bFocusIfOpen =*/false);
		check(EditorInst != nullptr);
		EditorInst->CloseWindow(EAssetEditorCloseReason::AssetUnloadingOrInvalid);
	}
	else
	{
		// Auto-import any namespaces we've collected as a result of compound events that may have occurred within this frame.
		if (!DeferredNamespaceImports.IsEmpty())
		{
			FImportNamespaceExParameters Params;
			Params.NamespacesToImport = MoveTemp(DeferredNamespaceImports);
			ImportNamespaceEx(Params);

			// Assert that this was reset by the move above.
			check(DeferredNamespaceImports.IsEmpty());
		}
	}
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::OnDropActors(const TArray<TWeakObjectPtr<AActor>>& Actors, UEdGraph* Graph, const UE::Math::TVector2<float>& DropLocation) const
{
	UE_LOG(LogBango, Display, TEXT("Test 2"));
	
	FDeprecateSlateVector2D NodeLocation = DropLocation;
	
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		AActor* DroppedActor = Actors[i].Get();
		if (DroppedActor && /*(DroppedActor->GetLevel() == BlueprintLevel) &&*/ !DroppedActor->IsChildActor())
		{
			UK2Node_BangoFindActor* ActorFindNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_BangoFindActor>(
				Graph, 
				NodeLocation, 
				EK2NewNodeFlags::SelectNewNode, 
				[DroppedActor] (UK2Node_BangoFindActor* NewNode)
				{
					NewNode->SetActor(DroppedActor);
				});
		}
	}
}

// ----------------------------------------------

FGraphAppearanceInfo FBangoScriptBlueprintEditor::GetGraphAppearance(class UEdGraph* InGraph) const
{
	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;

	FText OwnerNameText = GetOwnerNameAsText();
	FText LevelNameText = GetLevelNameAsText();
	
	FText InstructionText = OwnerNameText.IsEmpty() ? OwnerNameText : FText::Format(INVTEXT("{0} \U00002014 {1}"), { OwnerNameText, LevelNameText.IsEmpty() ? LOCTEXT("ScriptBlueprintEditor_NoLevel", "No Level") : LevelNameText }) ;
	FText CornerText = LevelNameText.IsEmpty() ? LOCTEXT("ScriptBlueprintEditor_ContentScript", "Bango Script") : LOCTEXT("ScriptBlueprintEditor_LevelScript", "Bango Level Script");
	
	AppearanceInfo.CornerText = CornerText;
	AppearanceInfo.InstructionText = InstructionText;
	AppearanceInfo.PIENotifyText = GetPIEStatus();
	AppearanceInfo.WarningText = WarningText;

	const float StartTimeNoFade = 2.0f;
	const float FadeDuration = 4.0f;
	const float MinFade = 0.15f;
	
	float Alpha = FMath::LerpStable(1.0f, 0.0f, (OpenTime - StartTimeNoFade) / FadeDuration);
	Alpha = FMath::Clamp(Alpha, MinFade, 1.0f);
	
	AppearanceInfo.InstructionFade = Alpha; 
	
	// Doesn't do anything. Unimplemented UE feature.
	//AppearanceInfo.CornerImage = ???

	return AppearanceInfo;
}

// ----------------------------------------------

FText FBangoScriptBlueprintEditor::GetLevelNameAsText() const
{
	const UBangoScriptBlueprint* Blueprint = GetBangoScriptBlueprintObj();
	
	if (!Blueprint)
	{
		return FText::GetEmpty();
	}
	
	TSoftObjectPtr<AActor> Actor = Blueprint->GetOwnerActor();
	
	if (Actor.IsNull())
	{
		return FText::GetEmpty();
	}
	else if (Actor.IsValid())
	{
		UPackage* LevelPackage = Actor->GetLevel()->GetPackage();
		return FText::FromString(FPackageName::GetShortName(LevelPackage) + FPackageName::GetMapPackageExtension());
	}
	else
	{
		// Try to get it from world partition
		UWorld* World = GEditor->EditorWorld;
		
		if (World)
		{
			UWorldPartition* WorldPartition = World->GetWorldPartition();
	
			if (WorldPartition)
			{
				UActorDescContainerInstance* ActorDescContainer = WorldPartition->GetActorDescContainerInstance();
		
				if (ActorDescContainer)
				{
					const FWorldPartitionActorDescInstance* ActorDesc = ActorDescContainer->GetActorDescInstanceByPath(Actor.ToSoftObjectPath());
			
					if (ActorDesc)
					{
						UPackage* LevelPackage = World->PersistentLevel.GetPackage();
						return FText::FromString(FPackageName::GetShortName(LevelPackage) + FPackageName::GetMapPackageExtension());
					}
				}
			}
		}
		
		// Unknown/unloaded fallback
		return LOCTEXT("OwnerName_UnloadedActor", "Unloaded Level");
	}
}

// ----------------------------------------------

FText FBangoScriptBlueprintEditor::GetOwnerNameAsText() const
{
	const UBangoScriptBlueprint* Blueprint = GetBangoScriptBlueprintObj();
	
	if (!Blueprint)
	{
		return FText::GetEmpty();
	}
	
	TSoftObjectPtr<AActor> Actor = Blueprint->GetOwnerActor();
	
	if (Actor.IsNull())
	{
		return FText::GetEmpty();
	}
	else if (Actor.IsValid())
	{
		return FText::FromString(Actor->GetActorLabel());
	}
	else
	{
		// Try to get it from world partition
		UWorld* World = GEditor->EditorWorld;
		
		if (World)
		{
			UWorldPartition* WorldPartition = World->GetWorldPartition();
	
			if (WorldPartition)
			{
				UActorDescContainerInstance* ActorDescContainer = WorldPartition->GetActorDescContainerInstance();
		
				if (ActorDescContainer)
				{
					const FWorldPartitionActorDescInstance* ActorDesc = ActorDescContainer->GetActorDescInstanceByPath(Actor.ToSoftObjectPath());
			
					if (ActorDesc)
					{
						return FText::FromName(ActorDesc->GetActorLabel());
					}
				}
			}
		}
		
		// Unknown/unloaded fallback
		return LOCTEXT("OwnerName_UnloadedActor", "Unloaded Actor");
	}
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::SetWarningText(const FText& InText)
{
	WarningText = InText;
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::InitBangoBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	check(InBlueprints.Num() == 1 || bShouldOpenInDefaultsMode);

	// TRUE if a single Blueprint is being opened and is marked as newly created
	bool bNewlyCreated = InBlueprints.Num() == 1 && InBlueprints[0]->bIsNewlyCreated;

	// Load editor settings from disk.
	// LoadEditorSettings();

	TArray< UObject* > Objects;
	for (UBlueprint* Blueprint : InBlueprints)
	{
		// Flag the blueprint as having been opened
		Blueprint->bIsNewlyCreated = false;

		Objects.Add( Blueprint );
	}
	
	if (!Toolbar.IsValid())
	{
		Toolbar = MakeShareable(new FBlueprintEditorToolbar(SharedThis(this)));
	}

	GetToolkitCommands()->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	CreateDefaultCommands();

	RegisterMenus();

	// Initialize the asset editor and spawn nothing (dummy layout)
	//const bool bCreateDefaultStandaloneMenu = true;
	//const bool bCreateDefaultToolbar = true;
	//const FName BlueprintEditorAppName = FName(TEXT("BlueprintEditorApp"));
	// InitAssetEditor(Mode, InitToolkitHost, BlueprintEditorAppName, FTabManager::FLayout::NullLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Objects);

	///
	AddEditingObject( InBlueprints[0] );
	///
	
	CommonInitialization(InBlueprints, bShouldOpenInDefaultsMode);

	InitalizeExtenders();

	RegenerateMenusAndToolbars();

	RegisterApplicationModes(InBlueprints, bShouldOpenInDefaultsMode, bNewlyCreated);

	// Post-layout initialization
	// PostLayoutBlueprintEditorInitialization();

	// Find and set any instances of this blueprint type if any exists and we are not already editing one
	FBlueprintEditorUtils::FindAndSetDebuggableBlueprintInstances();

	if ( bNewlyCreated )
	{
		if ( UBlueprint* Blueprint = GetBlueprintObj() )
		{
			if ( Blueprint->BlueprintType == BPTYPE_MacroLibrary )
			{
				NewDocument_OnClick(CGT_NewMacroGraph);
			}
			else if ( Blueprint->BlueprintType == BPTYPE_Interface )
			{
				NewDocument_OnClick(CGT_NewFunctionGraph);
			}
			else if ( Blueprint->BlueprintType == BPTYPE_FunctionLibrary )
			{
				NewDocument_OnClick(CGT_NewFunctionGraph);
			}
		}
	}

	if ( UBlueprint* Blueprint = GetBlueprintObj() )
	{
		if ( Blueprint->GetClass() == UBlueprint::StaticClass() && Blueprint->BlueprintType == BPTYPE_Normal )
		{
			if ( !bShouldOpenInDefaultsMode )
			{
				GetToolkitCommands()->ExecuteAction(FFullBlueprintEditorCommands::Get().EditClassDefaults.ToSharedRef());
			}
		}

		// There are upgrade notes, open the log and dump the messages to it
		if (Blueprint->UpgradeNotesLog.IsValid())
		{
			DumpMessagesToCompilerLog(Blueprint->UpgradeNotesLog->Messages, true);
		}
	}
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::AddEditingObject(UObject* Object)
{
	FBlueprintEditor::AddEditingObject(Object);
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::SetCurrentMode(FName NewMode)
{
	FBlueprintEditor::SetCurrentMode(NewMode);
	//SetUISelectionState(NAME_None);
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::PostInitAssetEditor()
{
	FBlueprintEditor::PostInitAssetEditor();
	
	OpenTime = 0.0f;
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::PasteGeneric()
{
	FBlueprintEditor::PasteGeneric();
}

// ----------------------------------------------

bool FBangoScriptBlueprintEditor::CanPasteGeneric() const
{
	return FBlueprintEditor::CanPasteGeneric();
}

// ----------------------------------------------

bool FBangoScriptBlueprintEditor::CanPasteNodes() const
{
	// Check if the clipboard contains actors in a map
	
	struct FClipboardActor
	{
		FString CastTo;
		FString TargetActor;
	};
	
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	const TCHAR* Buffer = *ClipboardContent;

	FParse::Next( &Buffer );

	int32 NestedDepth     = 0;

	FString StrLine;

	TArray<FClipboardActor> CopiedActors;
	
	while (FParse::Line(&Buffer, StrLine))
	{
		const TCHAR* Str = *StrLine;
		
		if (NestedDepth == 0)
		{
			const TCHAR* MAP = TEXT("MAP");
			if (GetBEGIN(&Str, MAP))
			{
				++NestedDepth;
			}
			else if (GetEND(&Str, MAP))
			{
				--NestedDepth;
			}
		}
		
		else if (NestedDepth == 1)
		{
			const TCHAR* LEVEL = TEXT("LEVEL");
			if (GetBEGIN(&Str, LEVEL))
			{
				++NestedDepth;
			}
			else if (GetEND(&Str, LEVEL))
			{
				--NestedDepth;
			}
		}
		
		else if (NestedDepth == 2)
		{
			const TCHAR* ACTOR = TEXT("ACTOR");
			if (GetBEGIN(&Str, ACTOR))
			{
				++NestedDepth;
				
				FString ExportPath;
				
				if (FParse::Value(Str, TEXT("ExportPath="), ExportPath))
				{
					FString ObjectClassName;
					FString TargetActor;
					
					if (FPackageName::ParseExportTextPath(ExportPath, &ObjectClassName, &TargetActor))
					{
						FString CastTo;
						if (ObjectClassName.EndsWith(TEXT("_C")))
						{
							CastTo = TEXT("/Script/Engine.BlueprintGeneratedClass"); 
						}
						else
						{
							CastTo = TEXT("/Script/CoreUObject.Class");
						}
						
						CastTo = FString::Format(TEXT("{0}'{1}'"), { CastTo, ObjectClassName } );
						
						CopiedActors.Add( {CastTo, TargetActor} );
					}
				}
			}
			else if (GetEND(&Str, ACTOR))
			{
				--NestedDepth;
			}
		}
		
		else
		{
			const TCHAR* OBJECT = TEXT("OBJECT");
			if (GetBEGIN(&Str, OBJECT))
			{
				++NestedDepth;
			}
			else if (GetEND(&Str, OBJECT))
			{
				--NestedDepth;
			}
		}
	}
	
	if (CopiedActors.Num() > 0)
	{
		return true;
	}
	
	return FBlueprintEditor::CanPasteNodes();
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::PasteNodesHere(UEdGraph* DestinationGraph, const FVector2f& GraphLocation)
{
	struct FClipboardActor
	{
		FString CastTo;
		FString TargetActor;
	};
	
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	const TCHAR* Buffer = *ClipboardContent;

	FParse::Next( &Buffer );

	int32 NestedDepth     = 0;

	FString StrLine;

	TArray<FClipboardActor> CopiedActors;
	
	const TCHAR* MAP = TEXT("MAP");
	const TCHAR* LEVEL = TEXT("LEVEL");
	const TCHAR* ACTOR = TEXT("ACTOR");
	const TCHAR* OBJECT = TEXT("OBJECT");
	
	while (FParse::Line(&Buffer, StrLine))
	{
		const TCHAR* Str = *StrLine;
		
		if (NestedDepth == 0)
		{
			// Only detect new Map openings
			if (GetBEGIN(&Str, MAP))
			{
				++NestedDepth;
			}
		}
		
		else if (NestedDepth == 1)
		{
			// Only detect new Level openings or closures of Map
			if (GetBEGIN(&Str, LEVEL))
			{
				++NestedDepth;
			}
			else if (GetEND(&Str, MAP))
			{
				--NestedDepth;
			}
		}
		
		else if (NestedDepth == 2)
		{
			// Only detect new Actor openings or closures of Level
			if (GetBEGIN(&Str, ACTOR))
			{
				++NestedDepth;
				
				FString ExportPath;
				
				if (FParse::Value(Str, TEXT("ExportPath="), ExportPath))
				{
					FString ObjectClassName;
					FString TargetActor;
					
					if (FPackageName::ParseExportTextPath(ExportPath, &ObjectClassName, &TargetActor))
					{
						// TODO: see if the engine has an existing function for this
						auto UnfixPIE = [] (const FString& PIEPath) -> FString
						{
							if (!GEditor->IsPlaySessionInProgress())
							{
								// Do nothing if we're not in PIE
								return PIEPath;
							}
						
							FString CorrectPath = PIEPath;
							
							FRegexPattern UEDPIEPattern(FString("^/Memory/UEDPIE_(\\d+)_"));
							FRegexMatcher UEDPIEMatcher(UEDPIEPattern, PIEPath);
						
							FRegexPattern GuidPattern(FString("_[a-zA-Z0-9]*\\."));
							FRegexMatcher GuidMatcher(GuidPattern, PIEPath);
						
							if (UEDPIEMatcher.FindNext() && GuidMatcher.FindNext())
							{
								int32 PIEBegin = UEDPIEMatcher.GetMatchBeginning();
								check(PIEBegin == 0);
							
								int32 PIEEnd = UEDPIEMatcher.GetMatchEnding();
								int32 PIERemoved = PIEEnd - PIEBegin;
								
								int32 GuidBegin =  GuidMatcher.GetMatchBeginning() - PIERemoved;
								int32 GuidEnd = GuidMatcher.GetMatchEnding() - PIERemoved;
								
								CorrectPath = CorrectPath.RightChop(PIEEnd);
								CorrectPath = CorrectPath.Left(GuidBegin) + TEXT(".") + CorrectPath.RightChop(GuidEnd);
								CorrectPath = TEXT("/Game/") + CorrectPath;
								
								return CorrectPath;
							}

							return PIEPath;	
						};
					
						TargetActor = UnfixPIE(TargetActor);
						
						FString CastTo;
						if (ObjectClassName.EndsWith(TEXT("_C")))
						{
							CastTo = TEXT("/Script/Engine.BlueprintGeneratedClass"); 
						}
						else
						{
							CastTo = TEXT("/Script/CoreUObject.Class");
						}
						
						CastTo = FString::Format(TEXT("{0}'{1}'"), { CastTo, ObjectClassName } );
						
						UE_LOG(LogBangoEditor, Display, TEXT("CastTo: %s    ObjectClassName: %s"), *CastTo, *ObjectClassName);
						
						CopiedActors.Add( {CastTo, TargetActor} );
					}
				}
			}
			else if (GetEND(&Str, LEVEL))
			{
				--NestedDepth;
			}
		}
		
		else if (NestedDepth == 3)
		{
			// Only detect new Object openings or closures of Actor
			if (GetBEGIN(&Str, OBJECT))
			{
				++NestedDepth;
			}
			else if (GetEND(&Str, ACTOR))
			{
				--NestedDepth;
			}
			else if (GetEND(&Str, OBJECT))
			{
				--NestedDepth;
			}
		}
		
		else
		{
			// Only detect closures of Object
			if (GetEND(&Str, OBJECT))
			{
				--NestedDepth;
			}
		}
	}
	
	if (CopiedActors.Num() > 0) 
	{
		const FString Format = TEXT
		(
			"Begin Object Class={0}\n"	// {/Script/BangoUncooked.K2Node_BangoFindActor}
				"CastTo={1}\n"			// "/Script/Engine.BlueprintGeneratedClass'/Game/TestThings/BP_StreetLamp.BP_StreetLamp_C'"
				"TargetActor={2}\n"		// "/Game/Test3.Test3:PersistentLevel.BP_StreetLamp_C_UAID_B04F13D324A438B102_1405045264"
			"End Object\n\n"
		);

		const FString FindActorNode = UK2Node_BangoFindActor::StaticClass()->GetClassPathName().ToString();
		
		TStringBuilder<512> ClipboardStringBuilder;
		
		for (const FClipboardActor& CopiedActor : CopiedActors)
		{
			ClipboardStringBuilder.Append(FString::Format(*Format, { FindActorNode, CopiedActor.CastTo, CopiedActor.TargetActor }));
		}
		
		FPlatformApplicationMisc::ClipboardCopy(*ClipboardStringBuilder);
	}
	
	// We delay it by one frame because sometimes Windows sucks and it takes a bit for the clipboard to update into the PasteNodesHere func
	TWeakObjectPtr<UEdGraph> WeakGraph = DestinationGraph;
	
	auto DelayedCall = [this, WeakGraph, GraphLocation] ()
	{
		if (UEdGraph* DestinationGraph = WeakGraph.Get())
		{
			FBlueprintEditor::PasteNodesHere(DestinationGraph, GraphLocation);
		}
	};
	
	GEditor->GetTimerManager()->SetTimerForNextTick(DelayedCall);
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::OnBangoScriptRan(UBangoScript* ScriptInstance)
{
	UBangoScriptBlueprint* RunningBlueprint = UBangoScriptBlueprint::GetBangoScriptBlueprintFromClass(TSoftClassPtr<UBangoScript>(ScriptInstance->GetClass()));
	
	const TArray<UObject*>* CurrentObjects = (this) ? GetObjectsCurrentlyBeingEdited() : nullptr;
	
	if (!CurrentObjects || CurrentObjects->Num() != 1)
	{
		return;
	}
	
	UBangoScriptBlueprint* ThisBlueprint = Cast<UBangoScriptBlueprint>((*CurrentObjects)[0]);
	
	if (RunningBlueprint == ThisBlueprint)
	{
		if (!ThisBlueprint->GetCurrentObjectBeingDebugged().IsValid())
		{
			ThisBlueprint->SetObjectBeingDebugged(ScriptInstance);
		}
	}
}

// ----------------------------------------------

void FBangoScriptBlueprintEditor::OnBangoScriptFinished(UBangoScript* ScriptInstance)
{
	
}

UBangoScriptBlueprint* FBangoScriptBlueprintEditor::GetBangoScriptBlueprintObj()
{
	return Cast<UBangoScriptBlueprint>(GetBlueprintObj());
}

const UBangoScriptBlueprint* FBangoScriptBlueprintEditor::GetBangoScriptBlueprintObj() const
{
	return Cast<UBangoScriptBlueprint>(GetBlueprintObj());
}

// ----------------------------------------------

bool FBangoScriptBlueprintEditor::GetBEGIN(const TCHAR** Stream, const TCHAR* Match) const
{
	const TCHAR* Original = *Stream;

	if( FParse::Command(Stream, TEXT("BEGIN")) && FParse::Command(Stream, Match))
	{
		return true;
	}
	
	*Stream = Original;
	
	return false;
}

// ----------------------------------------------

bool FBangoScriptBlueprintEditor::GetEND(const TCHAR** Stream, const TCHAR* Match) const
{
	const TCHAR* Original = *Stream;
	
	if (FParse::Command(Stream, TEXT("END")) && FParse::Command(Stream, Match))
	{
		return true;
	}
	
	*Stream = Original;

	return false;
}

#undef LOCTEXT_NAMESPACE
