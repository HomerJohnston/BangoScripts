#include "BangoLevelScriptsEditorSubsystem.h"

#include "ContentBrowserDataSubsystem.h"
#include "IContentBrowserDataModule.h"
#include "ISourceControlModule.h"
#include "K2Node_CustomEvent.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Core/BangoScriptContainer.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "BangoScripts/EditorTooling/BangoScriptHelperSubsystem.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Private/Unsorted/BangoDummyObject.h"
#include "Private/Menus/BangoEditorMenus.h"
#include "Private/Utilities/BangoEditorUtility.h"
#include "Private/Utilities/BangoFolderUtility.h"
#include "Private/Unsorted/BangoHideScriptFolderFilter.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "EdGraph/EdGraph.h"
#include "Factories/Factory.h"
#include "Misc/TransactionObjectEvent.h"
#include "Settings/EditorStyleSettings.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UObject/ObjectSaveContext.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ==============================================

TSharedPtr<IContentBrowserHideFolderIfEmptyFilter> UBangoLevelScriptsEditorSubsystem::Filter;

// ==============================================

UBangoLevelScriptsEditorSubsystem* UBangoLevelScriptsEditorSubsystem::Get()
{
	return GEditor->GetEditorSubsystem<UBangoLevelScriptsEditorSubsystem>();
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UContentBrowserDataSubsystem>();

	// Filter causes __BangoScripts__ folder to be hidden when it contains no "valid" assets (valid assets require a normal name; this is why I prepend level scripts with a ~ symbol)
	Filter = MakeShared<FBangoHideScriptFolderFilter>();
	UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
	ContentBrowserData->RegisterCreateHideFolderIfEmptyFilter(FContentBrowserCreateHideFolderIfEmptyFilter::CreateLambda([this] () { return Filter; }));

	// While maps are loading we want to ignore all script creation/destruction requests
	FEditorDelegates::OnMapLoad.AddUObject(this, &ThisClass::OnMapLoad);
	FEditorDelegates::OnMapOpened.AddUObject(this, &ThisClass::OnMapOpened);
	
	// Used to help detect "real" duplications of UBangoScriptComponent
	FEditorDelegates::OnDuplicateActorsBegin.AddWeakLambda(this, [this] () { bDuplicatingActors = true; } );
	FEditorDelegates::OnDuplicateActorsEnd.AddWeakLambda(this, [this] () { bDuplicatingActors = false; } );
	
	// Called by UBangoScriptComponent (potentially others)
	FBangoEditorDelegates::OnScriptContainerCreated.AddUObject(this, &ThisClass::OnLevelScriptContainerCreated);
	FBangoEditorDelegates::OnScriptContainerDestroyed.AddUObject(this, &ThisClass::OnLevelScriptContainerDestroyed);
	FBangoEditorDelegates::OnScriptContainerDuplicated.AddUObject(this, &ThisClass::OnLevelScriptContainerDuplicated);
	
	FCoreUObjectDelegates::OnObjectRenamed.AddUObject(this, &ThisClass::OnObjectRenamed);
	FCoreUObjectDelegates::OnObjectTransacted.AddUObject(this, &ThisClass::OnObjectTransacted);
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::Deinitialize()
{
	FEditorDelegates::OnMapLoad.RemoveAll(this);
	FEditorDelegates::OnMapOpened.RemoveAll(this);
	
	FEditorDelegates::OnDuplicateActorsBegin.RemoveAll(this);
	FEditorDelegates::OnDuplicateActorsEnd.RemoveAll(this);
	
	FBangoEditorDelegates::OnScriptContainerCreated.RemoveAll(this);
	FBangoEditorDelegates::OnScriptContainerDestroyed.RemoveAll(this);
	FBangoEditorDelegates::OnScriptContainerDuplicated.RemoveAll(this);
	
	FCoreUObjectDelegates::OnObjectRenamed.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectTransacted.RemoveAll(this);
	
	Super::Deinitialize();
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnObjectTransacted(UObject* Object, const class FTransactionObjectEvent& TransactionEvent)
{
	// TODO I need a way to allow hooking in other types more nicely. What if a user wants a custom type and not just UBangoScriptComponent?
	if (TransactionEvent.GetEventType() != ETransactionObjectEventType::UndoRedo)
	{
		return;
	}
	
	IBangoScriptHolderInterface* ScriptHolder = Cast<IBangoScriptHolderInterface>(Object);
	
	// do NOT check IsValid here. Undoing creation will have an invalid object.
	if (!ScriptHolder)
	{
		return;
	}
	
	UE_LOG(LogBangoEditor, Verbose, TEXT("OnObjectTransacted: %s, %i"), *Object->GetName(), (uint8)TransactionEvent.GetEventType());
	
	if (!Bango::Editor::IsComponentInEditedLevel(ScriptHolder->_getUObject()))
	{
		EnqueueDestroyedScriptContainer(*ScriptHolder);
	}
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap)
{
	bMapLoading = true;
	
	// The intent is to obliterate "undo" soft delete script assets after a new map is loaded in.
	auto DelayCollectGarbage = FTimerDelegate::CreateLambda([] ()
	{
		CollectGarbage(RF_NoFlags);	
	});
	
	GEditor->GetTimerManager()->SetTimerForNextTick(DelayCollectGarbage);
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnMapOpened(const FString& String, bool bArg)
{
	bMapLoading = false;
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnObjectRenamed(UObject* RenamedObject, UObject* RenamedObjectOuter, FName OldName) const
{
	if (!GEditor || !GEditor->IsInitialized())
	{
		return;
	}
    
	if (IsRunningCommandlet())
	{
		return;
	}
	
	// TODO I need a way to allow hooking in other types more nicely. What if a user wants a custom type and not just UBangoScriptComponent?
	// Should I have a C++ interface that other things could hook into?
	
	// This happens when a copied component is pasted
	if (RenamedObject->GetFlags() == RF_Transactional)
	{
		return;
	}
	
	TWeakObjectPtr<UObject> WeakRenamedObject = RenamedObject;
	TWeakObjectPtr<UObject> WeakRenamedObjectOuter = RenamedObjectOuter;
	TWeakObjectPtr<const UBangoLevelScriptsEditorSubsystem> WeakThis = this;
	
	auto DelayedScriptRename = [WeakRenamedObject, WeakRenamedObjectOuter, WeakThis, OldName] ()
	{
		UObject* RenamedObject = WeakRenamedObject.Get();
		UObject* RenamedObjectOuter = WeakRenamedObjectOuter.Get();
		const UBangoLevelScriptsEditorSubsystem* This = WeakThis.Get();
		
		if (!RenamedObject || !RenamedObjectOuter || !This)
		{
			return;
		}
		
		if (IBangoScriptHolderInterface* ScriptHolder = Cast<IBangoScriptHolderInterface>(RenamedObject))
		{
			if (!IsValid(RenamedObject) || RenamedObject->HasAnyFlags(RF_ArchetypeObject) || !Bango::Editor::IsComponentInEditedLevel(ScriptHolder->_getUObject()))
			{
				return;
			}
	
			UE_LOG(LogBangoEditor, Verbose, TEXT("OnObjectRenamed: %s, %s, %s"), *RenamedObject->GetName(), *RenamedObjectOuter->GetName(), *OldName.ToString());
			
			UBangoScriptBlueprint* Blueprint = UBangoScriptBlueprint::GetBangoScriptBlueprintFromClass(ScriptHolder->GetScriptContainer().GetScriptClass());

			if (Blueprint)
			{
				(void)Blueprint->MarkPackageDirty();
				
				FString NewPrivateName = Bango::Editor::GetLocalScriptName(ScriptHolder->_getUObject()->GetName());
				Blueprint->Rename(*NewPrivateName, nullptr, REN_DontCreateRedirectors | REN_NonTransactional);

				Blueprint->Modify();
				FKismetEditorUtilities::CompileBlueprint(Blueprint);
				
				ScriptHolder->_getUObject()->Modify();
				ScriptHolder->GetScriptContainer().SetScriptClass(Blueprint->GeneratedClass);
				
				// Tell details panels / script component customization to refresh
				FBangoEditorDelegates::OnScriptGenerated.Broadcast();
			}
		}
	};
	
	GEditor->GetTimerManager()->SetTimerForNextTick(DelayedScriptRename);
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnLevelScriptContainerCreated(IBangoScriptHolderInterface& ScriptHolder, FString BlueprintName)
{
	FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();
	
	UObject* Outer = ScriptHolder._getUObject();
	UE_LOG(LogBangoEditor, Verbose, TEXT("OnLevelScriptContainerCreated: %s, %s, %s"), *Outer->GetName(), *ScriptContainer.GetGuid().ToString(), *BlueprintName);

	ScriptContainer.SetRequestedName(BlueprintName);
	
	EnqueueCreatedScriptContainer(ScriptHolder);
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnLevelScriptContainerDestroyed(IBangoScriptHolderInterface& ScriptHolder)
{
	UObject* Outer = ScriptHolder._getUObject();
	UE_LOG(LogBangoEditor, Verbose, TEXT("OnLevelScriptContainerDestroyed: %s"), *Outer->GetName());

	EnqueueDestroyedScriptContainer(ScriptHolder);
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::OnLevelScriptContainerDuplicated(IBangoScriptHolderInterface& ScriptHolder)
{
	UObject* Outer = ScriptHolder._getUObject();
	FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();
	UE_LOG(LogBangoEditor, Verbose, TEXT("OnLevelScriptContainerDuplicated: %s, %s"), *Outer->GetName(), *ScriptContainer.GetGuid().ToString());
	
	// Through experimentation, a *true* duplicated component will either have the Transactional flag OR it will be wrapped in actor duplication.
	
	if (Outer->HasAllFlags(RF_Transactional) || bDuplicatingActors)
	{
		ScriptContainer.SetIsDuplicate();
		EnqueueCreatedScriptContainer(ScriptHolder);	
	}
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::EnqueueCreatedScriptContainer(IBangoScriptHolderInterface& ScriptHolder)
{
	// UObject* Owner, FBangoScriptContainer* ScriptContainer
	if (GEditor->IsPlaySessionInProgress())
	{
		return;
	}
	
	if (IsRunningCommandlet())
	{
		return;
	}
	
	if (bMapLoading)
	{
		return;
	}
	
	UObject* Outer = ScriptHolder._getUObject();
	FBangoScriptContainer* ScriptContainer = &ScriptHolder.GetScriptContainer();
	
	if (!Outer || !ScriptContainer)
	{
		checkNoEntry();
		return;
	}
	
	UE_LOG(LogBangoEditor, Verbose, TEXT("EnqueueCreatedScriptComponent: %s (%p)"), *Outer->GetName(), ScriptContainer);
	
	FScriptContainerKey Key(ScriptHolder);
	CreationRequests.Add(Key);
	RequestScriptQueueProcessing();
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::EnqueueDestroyedScriptContainer(IBangoScriptHolderInterface& ScriptHolder)
{
	// UObject* Owner, FBangoScriptContainer* ScriptContainer
	if (GEditor->IsPlaySessionInProgress())
	{
		return;
	}
	
	if (IsRunningCommandlet())
	{
		return;
	}
	
	if (bMapLoading)
	{
		return;
	}
	
	UObject* Outer = ScriptHolder._getUObject();
	FBangoScriptContainer* ScriptContainer = &ScriptHolder.GetScriptContainer();
	
	if (!Outer || !ScriptContainer)
	{
		checkNoEntry();
		return;
	}
	
	if (!ScriptContainer->GetGuid().IsValid())
	{
		return;
	}
	
	if (ScriptContainer->GetScriptClass().IsNull())
	{
		return;
	}
	
	UE_LOG(LogBangoEditor, Verbose, TEXT("EnqueueDestroyedScriptComponent: %s (%p)"), *Outer->GetName(), ScriptContainer);
	
	TSoftClassPtr<UBangoScript> ScriptClass = ScriptContainer->GetScriptClass();
    
    if (!ScriptClass.IsNull())
    {
	    DestructionRequests.Add(ScriptContainer->GetScriptClass());
    }
    
	RequestScriptQueueProcessing();
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::RequestScriptQueueProcessing()
{
	TWeakObjectPtr<UBangoLevelScriptsEditorSubsystem> WeakThis = this;
			
	auto ProcessScriptRequestQueuesNextTick = FTimerDelegate::CreateLambda([WeakThis] ()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->ProcessScriptRequestQueues();
		}
	});
		
	if (!ProcessScriptRequestQueuesHandle.IsValid())
	{
		ProcessScriptRequestQueuesHandle = GEditor->GetTimerManager()->SetTimerForNextTick(ProcessScriptRequestQueuesNextTick);
	}
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::ProcessScriptRequestQueues()
{
	UE_LOG(LogBangoEditor, Verbose, TEXT("--------------------------------"))
	UE_LOG(LogBangoEditor, Verbose, TEXT("ProcessScriptRequestQueue %llu"), GFrameCounter)

	// This should always be running one frame later than the requests were queued
	
	for (const auto& Request : CreationRequests)
	{
		if (!Request.Outer.IsValid())
		{
			continue;
		}
		
		TSoftClassPtr<UBangoScript> ScriptClass = Request.ScriptContainer->GetScriptClass();

	    // We want creation requests to take priority; parse through creations first and have any creation requests cancel out any destruction requestss
	    if (!ScriptClass.IsNull())
	    {
	    	UE_LOG(LogBangoEditor, Verbose, TEXT("Creation request contained existing script: %s"), *ScriptClass->GetPathName());
    		DestructionRequests.Remove(ScriptClass);
	    }
		else
		{
			UE_LOG(LogBangoEditor, Verbose, TEXT("Creation request for new script"));
		}
	    
		ProcessCreatedScriptRequest(Request.Outer, Request.ScriptContainer);
	}
	
	for (const TSoftClassPtr<UBangoScript>& DestroyedScript : DestructionRequests)
	{
		if (DestroyedScript.IsNull())
		{
			UE_LOG(LogBangoEditor, Warning, TEXT("Destruction request for null script, this should never happen"));
			continue;
		}
		
		UE_LOG(LogBangoEditor, Verbose, TEXT("Destruction request for script: %s"), *DestroyedScript->GetPathName());
		ProcessDestroyedScriptRequest(DestroyedScript);
	}
	
	CreationRequests.Empty();
	DestructionRequests.Empty();
	ProcessScriptRequestQueuesHandle.Invalidate();
	
	UE_LOG(LogBangoEditor, Verbose, TEXT("--------------------------------"))
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::ProcessCreatedScriptRequest(TWeakObjectPtr<UObject> Owner, FBangoScriptContainer* ScriptContainer)
{
	if (!Owner.IsValid())
	{
		return;
	}

	IBangoScriptHolderInterface* ScriptHolder = Cast<IBangoScriptHolderInterface>(Owner.Get());
	check(ScriptHolder);
	
	TSoftClassPtr<UBangoScript> ExistingScriptClass = ScriptContainer->GetScriptClass();
	
	if (ExistingScriptClass.IsNull())
	{
		if (ScriptContainer->ConsumeNewLevelScriptRequest())
		{
			// This must be a fresh shiny brand new script container. Make a script for it!
			CreateLevelScript(*ScriptHolder);	
		}
	}
	else
	{
		if (ScriptContainer->ConsumeDuplicate())
		{
			DuplicateLevelScript(*ScriptHolder);
		}
		else
		{
			// This script container already has a script class assigned, *and* it wasn't a duplicate... 			
			FSoftObjectPath ScriptClassSoft(ScriptContainer->GetScriptClass().ToSoftObjectPath());
					
			bool bScriptExists = FPackageName::DoesPackageExist(ScriptClassSoft.GetLongPackageName());
			
			if (!bScriptExists)
			{
				// This must have been an undo-delete. Let's see if we can find the script class in the Transient Package.
				TryUndeleteScript(ScriptClassSoft, ScriptContainer);
			}
		}
	}
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::CreateLevelScript(IBangoScriptHolderInterface& ScriptHolder)
{
	// This function must be ran one frame *after* the script container is created. This gives UE time finish all serialization behavior.
	FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();
	UObject* Outer = ScriptHolder._getUObject();
	
	if (ScriptContainer.GetGuid().IsValid())
	{
		UE_LOG(LogBangoEditor, Verbose, TEXT("Skipping creation of level script; existing script container already has a Guid"));
		return;
	}
	else
	{
		Outer->Modify();
		
		// All level scripts are supposed to be tied to an actor, although it doesn't actually do anything at runtime, it is moreso an editor concept.
		AActor* Actor = Outer->GetTypedOuter<AActor>();
		
		FGuid NewScriptGuid = FGuid::NewGuid(); 
		
		UPackage* ScriptPackage = Bango::Editor::MakeLevelScriptPackage(Outer, NewScriptGuid);
		
		if (!ScriptPackage)
		{
			UE_LOG(LogBangoEditor, Error, TEXT("Failed to create package for new Bango level script asset! Perhaps a file system issue?"));
			return;
		}

		UE_LOG(LogBangoEditor, Display, TEXT("Created new Bango level script asset: %s"), *ScriptPackage->GetPathName());
		
		// Higher-level systems can fill in the requested name to get it passed in here for creation. This was a bit of a late-stage hack, not very "clean code", but it works fine
		FString NewBlueprintName = ScriptContainer.GetRequestedName();
		
		if (NewBlueprintName.IsEmpty())
		{
			NewBlueprintName = FPackageName::GetShortName(ScriptPackage);
		}
		
		// Make the actual script blueprint asset!
		UBangoScriptBlueprint* Blueprint = Bango::Editor::MakeLevelScript(ScriptPackage, NewBlueprintName, NewScriptGuid);
		
		Blueprint->Modify();
		Blueprint->SetOwnerActor(Actor);
		Blueprint->SetScriptHolder(ScriptHolder);
		
		if (!Blueprint)
		{
			UE_LOG(LogBangoEditor, Error, TEXT("Failed to create new level script!"))
			return;
		}
	
		
		UClass* GenClass = Blueprint->GeneratedClass;
		
		if (GenClass)
		{
			UObject* CDO = GenClass->GetDefaultObject();
			UBangoScript* BangoScript = Cast<UBangoScript>(CDO);
			
			if (BangoScript)
			{
				// This enables the "This" node to self-setup its own output cast type
				BangoScript->SetThis_ClassType(Actor->GetClass());
			}
		}
		
		ScriptContainer.SetGuid(NewScriptGuid);
		ScriptContainer.SetScriptClass(Blueprint->GeneratedClass);
	
		UEdGraph* EventGraph = Blueprint->UbergraphPages[0].Get();
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			Node->NodeComment = ScriptHolder.GetStartEventComment();
		}
		
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		FAssetRegistryModule::AssetCreated(Blueprint);
		(void)ScriptPackage->MarkPackageDirty();
	
		// Tells FBangoScript property type customizations to regenerate their view
		FBangoEditorDelegates::OnScriptGenerated.Broadcast();
		
		// Throw a dummy property change event. UBangoScriptComponent will use this to update its billboard.
		FPropertyChangedEvent NewScriptDummyChangedEvent(nullptr, EPropertyChangeType::Unspecified, {});
		Outer->PostEditChangeProperty(NewScriptDummyChangedEvent);
	}
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::DuplicateLevelScript(IBangoScriptHolderInterface& ScriptHolder)
{
	FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();
	UObject* Outer = ScriptHolder._getUObject();
	
	UBangoScriptBlueprint* SourceBlueprint = UBangoScriptBlueprint::GetBangoScriptBlueprintFromClass(ScriptContainer.GetScriptClass());

	if (!SourceBlueprint)
	{
		UE_LOG(LogBangoEditor, Error, TEXT("Failed to locate script blueprint asset for script class path %s"), *ScriptContainer.GetScriptClass().ToSoftObjectPath().ToString());
		return;
	}
	
	AActor* OwnerActor = Outer->GetTypedOuter<AActor>();
	
	if (!OwnerActor)
	{
		UE_LOG(LogBangoEditor, Error, TEXT("Failed to duplicate script; could not find an AActor outer for %s"), *Outer->GetPathName()); 
		return;
	}
		
	Outer->Modify();
	
	// Prepare the newly duplicated script container
	ScriptContainer.Unset();
		
	FGuid NewScriptGuid = FGuid::NewGuid(); 
	ScriptContainer.SetGuid(NewScriptGuid);
		
	// Duplicate the blueprint
	UPackage* NewScriptPackage = Bango::Editor::MakeLevelScriptPackage(Outer, NewScriptGuid);
		
	if (!NewScriptPackage)
	{
		UE_LOG(LogBango, Error, TEXT("Tried to create a new script but could not create a package!"));
		return;
	}
	
	UE_LOG(LogBangoEditor, Verbose, TEXT("OnLevelScriptContainerDuplicated: %s, %s, %s"), *Outer->GetName(), *ScriptContainer.GetGuid().ToString(), *ScriptContainer.GetRequestedName());
	
	UBangoScriptBlueprint* DuplicatedBlueprint = Bango::Editor::DuplicateLevelScript(SourceBlueprint, NewScriptPackage, ScriptContainer.GetRequestedName(), NewScriptGuid, OwnerActor);
	check(DuplicatedBlueprint);
	
	FKismetEditorUtilities::CompileBlueprint(DuplicatedBlueprint);
	
	Outer->Modify();
	ScriptContainer.SetScriptClass(DuplicatedBlueprint->GeneratedClass);
	
	FAssetRegistryModule::AssetCreated(DuplicatedBlueprint);
	(void)NewScriptPackage->MarkPackageDirty();
	
	// Tells FBangoScript property type customizations to regenerate
	FBangoEditorDelegates::OnScriptGenerated.Broadcast();
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::TryUndeleteScript(FSoftObjectPath ScriptClassSoft, FBangoScriptContainer* ScriptContainer)
{
	TArray<UObject*> TransientObjects;
    GetObjectsWithOuter(GetTransientPackage(), TransientObjects);

    UBangoScriptBlueprint* MatchedBlueprint = nullptr;
    
    for (UObject* TransientObject : TransientObjects)
    {
        if (UBangoScriptBlueprint* TransientBangoScriptBlueprint = Cast<UBangoScriptBlueprint>(TransientObject))
        {
            if (TransientBangoScriptBlueprint->DeletedPackagePath == ScriptClassSoft.GetAssetPath().GetPackageName().ToString())
            {
                if (FPackageName::DoesPackageExist(TransientBangoScriptBlueprint->DeletedPackagePath.GetLongPackageName()))
                {
                	UE_LOG(LogBango, Warning, TEXT("Tried to restore deleted Bango Blueprint but there was another package at the location. Invalidating this Script Container property."));
                	ScriptContainer->Unset();
                }
                else
                {
                	MatchedBlueprint = TransientBangoScriptBlueprint;
                	break;
                }
            }
        }
    }
    
    if (MatchedBlueprint)
    {
        UPackage* RestoredPackage = CreatePackage(*MatchedBlueprint->DeletedPackagePath.ToString());
        RestoredPackage->SetFlags(RF_Public);
        RestoredPackage->SetPackageFlags(PKG_NewlyCreated);
        RestoredPackage->SetPersistentGuid(MatchedBlueprint->DeletedPackagePersistentGuid);
        RestoredPackage->SetPackageId(MatchedBlueprint->DeletedPackageId);
                			
        MatchedBlueprint->Rename(*MatchedBlueprint->DeletedName, RestoredPackage, REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);
        MatchedBlueprint->Modify();
        MatchedBlueprint->ClearFlags(RF_Transient);
                			
        FAssetRegistryModule::AssetCreated(MatchedBlueprint);
        (void)MatchedBlueprint->MarkPackageDirty();
        
        if (RestoredPackage->GetPackageId().IsValid())
        {
            GEditor->GetEditorSubsystem<UEditorAssetSubsystem>()->SaveLoadedAsset(MatchedBlueprint, false);
        }
    }
    else
    {
        UE_LOG(LogBangoEditor, Warning, TEXT("Error - could not find associated blueprint for %s"), *ScriptClassSoft.ToString());
    }
}

// ----------------------------------------------

void UBangoLevelScriptsEditorSubsystem::ProcessDestroyedScriptRequest(TSoftClassPtr<UBangoScript> ScriptClass)
{
	const FSoftObjectPath& ScriptClassPath = ScriptClass.ToSoftObjectPath();
	
	if (ScriptClassPath.IsNull())
	{
		return;
	}
	
	UBangoScriptBlueprint* Blueprint = UBangoScriptBlueprint::GetBangoScriptBlueprintFromClass(ScriptClass); 

	if (!Blueprint)
	{
		UE_LOG(LogBangoEditor, Error, TEXT("SoftDeleteLevelScriptPackage failed - Null Blueprint: %s"), *ScriptClass.ToSoftObjectPath().GetAssetPathString());
		return;
	}
	
	UPackage* OldPackage = Blueprint->GetPackage();

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(Blueprint);

	Blueprint->ClearEditorReferences();

	// We're going to "softly" delete the blueprint. Stash its name & package path inside of it, and then rename it to its Guid form.
	// When the script container is restored (undo delete), it can look for the script inside the transient package by its Guid to restore it.
	Blueprint->DeletedName = Blueprint->GetName();
	Blueprint->DeletedPackagePath = Blueprint->GetPackage()->GetPathName();
	Blueprint->DeletedPackagePersistentGuid = OldPackage->GetPersistentGuid();
	Blueprint->DeletedPackageId = OldPackage->GetPackageId();
	
	Blueprint->Rename(*Blueprint->GetScriptGuid().ToString(), GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);
	
	// TODO Is there another way to make this work without this? I can't successfully run ObjectTools' delete funcs on a UPackage, only on an asset inside a UPackage!
	UBangoDummyObject* WhyDoINeedToPutADummyObjectIntoAPackageToDeleteIt = NewObject<UBangoDummyObject>(OldPackage);
	
	int32 NumDelete = ObjectTools::ForceDeleteObjects( { WhyDoINeedToPutADummyObjectIntoAPackageToDeleteIt }, false );
			
	if (NumDelete == 0)
	{
		ObjectTools::ForceDeleteObjects( { WhyDoINeedToPutADummyObjectIntoAPackageToDeleteIt } );
	}

	Bango::Editor::DeleteEmptyLevelScriptFolders();
}

// ----------------------------------------------

#undef LOCTEXT_NAMESPACE
