#include "BangoScripts/Components/BangoScriptComponent.h"

#include "IImageWrapper.h"
#include "TextureResource.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Subsystem/BangoScriptSubsystem.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Components/BillboardComponent.h"
#include "UObject/ICookInfo.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#endif

#define LOCTEXT_NAMESPACE "BangoScripts"

#if WITH_EDITOR
TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent)> UBangoScriptComponent::OnDebugDrawEditor;
TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent)> UBangoScriptComponent::OnDebugDrawPIE;
#endif

// ----------------------------------------------

UBangoScriptComponent::UBangoScriptComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

// ----------------------------------------------

void UBangoScriptComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (bRunOnBeginPlay)
	{	
		Run();
	}
}

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnRegister()
{
	Super::OnRegister();
	
	Bango::Debug::PrintComponentState(this, "OnRegister");
	
	if (Bango::Editor::IsComponentInEditedLevel(this))
	{
		FString ScriptName = GetName(); // We will use the component name for the script name
		FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(AsScriptHolder(), ScriptName);
	}
	
    if (IsTemplate())
    {
	    return;
    }
	
	if (GetWorld()->IsEditorWorld())
    {
		FEditorDelegates::PreBeginPIE.AddWeakLambda(this, [this] (const bool bIsSimulating)
		{
			if (bDebugRegistered)
			{
				FBangoEditorDelegates::ScriptComponentRegistered.Broadcast(this, EBangoScriptComponentRegisterStatus::Unregistered);
				bDebugRegistered = false;				
			}
		});
	
		FEditorDelegates::EndPIE.AddWeakLambda(this, [this] (const bool bIsSimulating)
		{
			if (!bDebugRegistered)
			{
				FBangoEditorDelegates::ScriptComponentRegistered.Broadcast(this, EBangoScriptComponentRegisterStatus::Registered);
				bDebugRegistered = true;		
			}
		});
		
		if (!bDebugRegistered)
		{
			FBangoEditorDelegates::ScriptComponentRegistered.Broadcast(this, EBangoScriptComponentRegisterStatus::Registered);
			bDebugRegistered = true;		
		}
    }
    
    if (GetWorld()->IsGameWorld())
    {
    	if (!bDebugRegistered)
    	{
    		FBangoEditorDelegates::ScriptComponentRegistered.Broadcast(this, EBangoScriptComponentRegisterStatus::Registered);
    		bDebugRegistered = true;		
    	}
    }
	
	if (!BillboardInstance && GetOwner() && !GetWorld()->IsGameWorld())
    {
        {
            FCookLoadScope EditorOnlyLoadScope(ECookLoadType::EditorOnly);
            const EObjectFlags TransactionalFlag = GetFlags() & RF_Transactional;
	
            BillboardInstance = NewObject<UBillboardComponent>(GetOwner(), NAME_None, TransactionalFlag | RF_Transient | RF_TextExportTransient);
        }
		
        BillboardInstance->SetupAttachment(GetOwner()->GetRootComponent());
        BillboardInstance->SetIsVisualizationComponent(true);
		BillboardInstance->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
        BillboardInstance->bHiddenInGame = true;
		BillboardInstance->bIsScreenSizeScaled = true;
        BillboardInstance->Mobility = EComponentMobility::Movable;
        BillboardInstance->AlwaysLoadOnClient = false;
		BillboardInstance->bIsEditorOnly = true;
        BillboardInstance->SpriteInfo.Category = TEXT("Misc");
        BillboardInstance->SpriteInfo.DisplayName = NSLOCTEXT("SpriteCategory", "Misc", "Misc");
        BillboardInstance->CreationMethod = CreationMethod;
        BillboardInstance->bUseInEditorScaling = true;
        BillboardInstance->OpacityMaskRefVal = .1f;

	    BillboardInstance->RegisterComponent();
    }
	
	UpdateBillboard();
	
	// Bango::Debug::PrintComponentState(this, "OnRegister_Late");
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnUnregister()
{
	Bango::Debug::PrintComponentState(this, "OnUnregister");
	
	if (bDebugRegistered)
	{
		FBangoEditorDelegates::ScriptComponentRegistered.Broadcast(this, EBangoScriptComponentRegisterStatus::Unregistered);
		bDebugRegistered = false;				
	}

	// Objects that are freshly duplicated/copied/etc will not have this flag. Prevents double-occurrences of processing.
	if (HasAllFlags(RF_WasLoaded))
	{
		FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(AsScriptHolder(), EBangoScriptDeletedHelper::OwnerDestroyed);
	}
	
	Super::OnUnregister();
	
	// Bango::Debug::PrintComponentState(this, "OnUnregister_Late");	
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnComponentCreated()
{
	Bango::Debug::PrintComponentState(this, "OnComponentCreated");
	
	Super::OnComponentCreated();
		
	if (Bango::Editor::IsComponentInEditedLevel(this))
	{
		// With RF_LoadCompleted this is a default actor component. We rely on PostDuplicated instead.
		if (!HasAllFlags(RF_LoadCompleted))
		{
			FString ScriptName = GetName(); // We will use the component name for the script name
	
			if (ScriptContainer.GetGuid().IsValid())
			{
				// FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
			}
			else
			{
				// FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(AsScriptHolder(), ScriptName);
			}	
		}
	
	}

	// Bango::Debug::PrintComponentState(this, "OnComponentCreated_Late");
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// TODO I currently use the component long after it is being destroyed here... is this safe? It isn't crashing but maybe it will sometimes.
	// I may need to store the target script class early and pass it over to the subsystem handler
	// TSoftClassPtr<UBangoScript> ScriptClass = ScriptContainer.GetScriptClass();
	
	Bango::Debug::PrintComponentState(this, "OnComponentDestroyed");
	
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	
	if (BillboardInstance)
	{
		BillboardInstance->DestroyComponent();
	}
	
	// Two paths because this engine is complex as hell and I am not certain that I can always treat these identically
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		//if (!HasAnyFlags(RF_WasLoaded))
		//{
			// Through trial and error testing, I know that RF_BeginDestroyed will be applied when the component is unloaded, but not when it is deleted. But only for CDO component. 
			if (!HasAnyFlags(RF_BeginDestroyed))
			{
				UE_LOG(LogBango, Verbose, TEXT("Calling OnScriptContainerDestroyed for Instance component..."));
				
				FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(AsScriptHolder(), EBangoScriptDeletedHelper::SuspectedDelete);
			}
		//}
	}
	else
	{ 
		if (bDestroyingHierarchy)
		{
			if (GetOwner() && GetOwner()->HasAnyFlags(RF_BeginDestroyed))
			{
				UE_LOG(LogBango, Verbose, TEXT("Calling OnScriptContainerDestroyed for CDO component (owner BeginDestroyed)..."));
				
				FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(AsScriptHolder(), EBangoScriptDeletedHelper::SuspectedDelete);
			}
			else if (!HasAnyFlags(RF_WasLoaded | RF_BeginDestroyed) || !IsValid(GetOwner()))
			{
				UE_LOG(LogBango, Verbose, TEXT("Calling OnScriptContainerDestroyed for CDO component..."));
				
				FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(AsScriptHolder(), EBangoScriptDeletedHelper::SuspectedDelete);
			}
		}
	}
}

void UBangoScriptComponent::PostApplyToComponent()
{
	Bango::Debug::PrintComponentState(this, "PostApplyToComponent");
	
	Super::PostApplyToComponent();
}

void UBangoScriptComponent::PostLoad()
{
	Bango::Debug::PrintComponentState(this, "PostLoad");
	
	Super::PostLoad();
	
	TWeakObjectPtr<UBangoScriptComponent> WeakThis = this;
	
	auto CheckScriptValidity = [WeakThis] ()
	{
		if (WeakThis.IsValid())
		{
			TSoftClassPtr<UBangoScript> ScriptClass = WeakThis->ScriptContainer.GetScriptClass();
		
			if (!ScriptClass.IsNull())
			{
				FName PackageName = ScriptClass.ToSoftObjectPath().GetLongPackageFName();
			
				if (!FPackageName::DoesPackageExist(PackageName.ToString()))
				{
					WeakThis->Modify();
					WeakThis->ScriptContainer.Unset();
				}
			}	
		}
	};
	
	// GEditor->GetTimerManager()->SetTimerForNextTick(CheckScriptValidity);
}

void UBangoScriptComponent::BeginDestroy()
{
	Bango::Debug::PrintComponentState(this, "BeginDestroy");
	
	Super::BeginDestroy();
}

void UBangoScriptComponent::FinishDestroy()
{
	Bango::Debug::PrintComponentState(this, "FinishDestroy");
	
	Super::FinishDestroy();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	
	// Ignore PIE. Ignore World (level) duplication.
	if (DuplicateMode != EDuplicateMode::Normal)
	{
		return;
	}
	if (!Bango::Editor::IsComponentInEditedLevel(this))
	{
		return;
	}
	
	Bango::Debug::PrintComponentState(this, "PostDuplicate");
	
	if (HasAnyFlags(RF_Transactional))
	{
		FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
		return;
	}
	
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		//Bango::Debug::PrintComponentState(this, "PostDuplicate_Instance");
		
		// Component was added to an actor in the level; this case is very easy to handle, PostDuplicate is only called on real human-initiated duplications
		FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
	}
	else
	{
		// Component is part of a blueprint, only run duplication code if it's in a level already
		// Owner will have WasLoaded if it is being loaded/unloaded as part of world partition
		if (!GetOwner()->HasAnyFlags(RF_WasLoaded))
		{
			FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
		}
	}
}

// ----------------------------------------------

#if WITH_EDITOR
#endif
void UBangoScriptComponent::PostEditImport()
{
	Bango::Debug::PrintComponentState(this, "PostEditImport");
	Super::PostEditImport();
	
	if (GetFlags() == RF_Transactional)
	{
		FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdateBillboard();
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UBangoScriptComponent::InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly)
{
	Super::InvalidateLightingCacheDetailed(bInvalidateBuildEnqueuedLighting, bTranslationOnly);
}

void UBangoScriptComponent::PreSave(FObjectPreSaveContext SaveContext)
{
	bSaving = true;
	
	Super::PreSave(SaveContext);
	
	UE_LOG(LogBango, Verbose, TEXT("PreSave {%s}"), *GetPathName());
	
	// The level scripts subsystem may be fixing this up after another tick. So let's save the referenced script after another tick, too.
	TWeakObjectPtr<UBangoScriptComponent> WeakThis = this;
	
	auto DelaySaveScript = [WeakThis]
	{
		if (!WeakThis.IsValid())
		{
			return;
		}
		
		TSoftClassPtr<UBangoScript> ScriptClass = WeakThis->GetScriptContainer().GetScriptClass();
		
		if (!ScriptClass.IsNull())
		{
			FName PackageName = ScriptClass.ToSoftObjectPath().GetLongPackageFName();
				
			if (!FPackageName::DoesPackageExist(PackageName.ToString()))
			{
				// The script class exists but it hasn't been saved yet. We need to fix this.
				FBangoEditorDelegates::RequestScriptSave.Broadcast(WeakThis.Get(), ScriptClass);
			}
		}
	};

	// GEditor->GetTimerManager()->SetTimerForNextTick(DelaySaveScript);
}

void UBangoScriptComponent::PreSaveRoot(FObjectPreSaveRootContext ObjectSaveContext)
{
	Super::PreSaveRoot(ObjectSaveContext);
}

void UBangoScriptComponent::PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext)
{
	Super::PostSaveRoot(ObjectSaveContext);
}
#endif

// ----------------------------------------------

void UBangoScriptComponent::Run()
{
#if WITH_EDITOR
	RunningHandle =  
#endif
	UBangoScriptSubsystem::EnqueueScript(ScriptContainer.GetScriptClass(), GetOwner(), ScriptContainer.GetPropertyBag());
	
#if WITH_EDITOR
	if (RunningHandle.IsRunning())
	{
		TDelegate<void(FBangoScriptHandle)> OnFinished = TDelegate<void(FBangoScriptHandle)>::CreateUObject(this, &ThisClass::OnScriptFinished);
		UBangoScriptSubsystem::RegisterOnScriptFinished(this, RunningHandle, OnFinished);
	}
#endif
}

// ----------------------------------------------

#if WITH_EDITOR
UBangoScriptBlueprint* UBangoScriptComponent::GetScriptBlueprint(bool bForceLoad) const
{
	TSoftClassPtr<UBangoScript> ScriptClass = ScriptContainer.GetScriptClass();
	
	if (ScriptClass.IsNull())
	{
		return nullptr;
	}
	
	if (ScriptClass.IsValid())
	{
		TSubclassOf<UBangoScript> LoadedScriptClass = ScriptClass.Get();
		return Cast<UBangoScriptBlueprint>(UBlueprint::GetBlueprintFromClass(LoadedScriptClass));
	}
	
	if (ScriptClass.IsPending() && bForceLoad)
	{
		TSubclassOf<UBangoScript> LoadedScriptClass = ScriptClass.LoadSynchronous();
		return Cast<UBangoScriptBlueprint>(UBlueprint::GetBlueprintFromClass(LoadedScriptClass));
	}

	return nullptr;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::SetScriptBlueprint(UBangoScriptBlueprint* Blueprint)
{
	if (!Blueprint->GeneratedClass->IsChildOf(UBangoScript::StaticClass()))
	{
		UE_LOG(LogBango, Error, TEXT("Tried to set blueprint but the blueprint given was not a UBangoScriptInstance!"));
		return;
	}
	
	Modify();
	(void)MarkPackageDirty();
	
	ScriptContainer.SetScriptClass(Cast<UClass>(Blueprint->GeneratedClass));
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnScriptFinished(FBangoScriptHandle FinishedHandle)
{
	if (FinishedHandle != RunningHandle)
	{
		return;
	}
	
	RunningHandle.Expire();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PreEditUndo()
{
	Bango::Debug::PrintComponentState(this, "PreEditUndo");

	Super::PreEditUndo();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PostEditUndo(TSharedPtr<ITransactionObjectAnnotation> TransactionAnnotation)
{
	Bango::Debug::PrintComponentState(this, "PostEditUndo");

	Super::Super::PostEditUndo(TransactionAnnotation);
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
FVector UBangoScriptComponent::GetDebugDrawOrigin() const
{
	if (BillboardSettings.bDisable)
	{
		return GetOwner()->GetActorLocation();
	}

	return GetBillboardOffset();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
FVector UBangoScriptComponent::GetBillboardPosition() const
{
	FVector OwnerPosition = GetOwner()->GetActorLocation();
	FVector CorrectedBillboardOffset = GetOwner()->GetTransform().InverseTransformVector(GetBillboardOffset());
	
	return OwnerPosition + CorrectedBillboardOffset;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
bool UBangoScriptComponent::HasValidScript() const
{
	return !ScriptContainer.GetScriptClass().IsNull();
}

void UBangoScriptComponent::LogStatus(FString* OutString) const
{
	FString String = FString::Format(TEXT("LOGSTATUS UBangoScriptComponent {{0}}, Script: {{1}}"), { *GetPathName(), (ScriptContainer.GetScriptClass().IsNull()) ? TEXT("NONE") : *ScriptContainer.GetScriptClass().ToString() } );
	
	if (OutString)
	{
		*OutString = String;
	}
	else
	{
		UE_LOG(LogBango, Verbose, TEXT("     %s"), *String);
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::UpdateBillboard()
{
	if (!BillboardInstance)
	{
		return;
	}
	
	if (BillboardSettings.bDisable)
	{
		BillboardInstance->SetVisibility(false);
		BillboardInstance->SetRelativeLocation(FVector::ZeroVector);
		return;
	}
	
	UTexture2D* BillboardSprite = Bango::Debug::GetScriptBillboardSprite(this, BillboardSettings.CustomBillboard);
	
	BillboardInstance->SetSprite(BillboardSprite);
	BillboardInstance->SetVisibility(true);
	
	FVector TransformedOffset = GetOwner()->GetTransform().InverseTransformVector(GetBillboardOffset());
	BillboardInstance->SetRelativeLocation(TransformedOffset);

	int32 BillboardSize = BillboardSprite->GetSizeX();
	const int32 SpriteSize = BillboardSize / 2;

	int32 U = 0;
	int32 V = 0;
	int32 UL = SpriteSize;
	int32 VL = SpriteSize;
	
	if (!ScriptContainer.GetScriptClass().IsNull())
	{
		U = SpriteSize;
	}
	
	if (bRunOnBeginPlay)
	{
		V = SpriteSize;
	}
	
	BillboardInstance->SetUV(U, UL, V, VL);	
}
#endif

// ----------------------------------------------

#undef LOCTEXT_NAMESPACE