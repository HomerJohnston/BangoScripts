#include "BangoScripts/Components/BangoScriptComponent.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "TextureResource.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Subsystem/BangoScriptSubsystem.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "BangoScripts_EditorTooling/BangoScripts_EditorTooling.h"
#include "Components/BillboardComponent.h"
#include "Fonts/FontMeasure.h"
#include "UObject/ICookInfo.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#endif

#define LOCTEXT_NAMESPACE "BangoScripts"

#if WITH_EDITOR
TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)> UBangoScriptComponent::OnDebugDrawEditor;
TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)> UBangoScriptComponent::OnDebugDrawPIE;
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
	
	Bango::Debug::PrintComponentState(this, "OnRegister_Early");
	
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
		// Unregister the editor instance from DebugDraw prior to starting PIE
		FEditorDelegates::PreBeginPIE.AddWeakLambda(this, [this] (const bool bIsSimulating) { FBangoEditorDelegates::DebugDrawRequest.RemoveAll(this); } );
		
		// Re-register the editor instance to DebugDraw when PIE ends
		FEditorDelegates::EndPIE.AddWeakLambda(this, [this] (const bool bIsSimulating) { FBangoEditorDelegates::DebugDrawRequest.AddUObject(this, &ThisClass::PerformDebugDrawUpdate); });
		
		// Start registered
	    FBangoEditorDelegates::DebugDrawRequest.AddUObject(this, &ThisClass::PerformDebugDrawUpdate);
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
	
	Bango::Debug::PrintComponentState(this, "OnRegister_Late");
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnUnregister()
{
	Bango::Debug::PrintComponentState(this, "OnUnregister_Early");
	
	FBangoEditorDelegates::DebugDrawRequest.RemoveAll(this);

	FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(AsScriptHolder());
		
	Super::OnUnregister();
	
	Bango::Debug::PrintComponentState(this, "OnUnregister_Late");	
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnComponentCreated()
{
	Bango::Debug::PrintComponentState(this, "OnComponentCreated_Early");
	
	Super::OnComponentCreated();
		
	if (Bango::Editor::IsComponentInEditedLevel(this))
	{
		// With RF_LoadCompleted this is a default actor component. We rely on PostDuplicated instead.
		if (!HasAllFlags(RF_LoadCompleted))
		{
			FString ScriptName = GetName(); // We will use the component name for the script name
	
			if (ScriptContainer.GetGuid().IsValid())
			{
				FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
			}
			else
			{
				FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(AsScriptHolder(), ScriptName);
			}	
		}
	
	}

	Bango::Debug::PrintComponentState(this, "OnComponentCreated_Late");
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	TSoftClassPtr<UBangoScript> ScriptClass = ScriptContainer.GetScriptClass();
	
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	
	if (BillboardInstance)
	{
		BillboardInstance->DestroyComponent();
	}
	
	if (!ScriptClass.IsNull())
	{
		// Moves handling over to an editor module to handle more complicated package deletion/undo management
		FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(AsScriptHolder());
	}
}

void UBangoScriptComponent::PostApplyToComponent()
{
	Super::PostApplyToComponent();
}

void UBangoScriptComponent::BeginDestroy()
{
	Bango::Debug::PrintComponentState(this, "BeginDestroy_Early");
	
	Super::BeginDestroy();
}

void UBangoScriptComponent::FinishDestroy()
{
	Bango::Debug::PrintComponentState(this, "FinishDestroy_Early");
	
	Super::FinishDestroy();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Bango::Debug::PrintComponentState(this, "PostDuplicate_Early");
	
	if (!Bango::Editor::IsComponentInEditedLevel(this))
	{
		return;
	}
	
	Bango::Debug::PrintComponentState(this, "PostDuplicate_InEditedLevel");
	
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		Bango::Debug::PrintComponentState(this, "PostDuplicate_Instance");
		
		// Component was added to an actor in the level; this case is very easy to handle, PostDuplicate is only called on real human-initiated duplications
		FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
	}
	else
	{
		Bango::Debug::PrintComponentState(this, "PostDuplicate_CDO");

		// Component is part of a blueprint, only run duplication code if it's in a level already
		//if (GetOwner()->HasAnyFlags(RF_WasLoaded))
		//{
			FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(AsScriptHolder());
		//}
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
		TSubclassOf<UBangoScript> LoadedScriptClass = ScriptClass.Get();
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
void UBangoScriptComponent::PerformDebugDrawUpdate(FBangoDebugDrawCanvas& Canvas, bool bPIE)
{
	// We delegate these requests over to the editor module to make it easier to build in editor functionality
	// See UBangoDebugDraw_ScriptComponent
	
	if (bPIE)
	{
		OnDebugDrawPIE.Broadcast(Canvas, this);
	}
	else
	{
		OnDebugDrawEditor.Broadcast(Canvas, this);
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PreEditUndo()
{
	Super::PreEditUndo();
	
	Bango::Debug::PrintComponentState(this, "PreEditUndo");
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PostEditUndo(TSharedPtr<ITransactionObjectAnnotation> TransactionAnnotation)
{
	Super::Super::PostEditUndo(TransactionAnnotation);
	
	Bango::Debug::PrintComponentState(this, "PostEditUndo");
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
	BillboardInstance->SetRelativeLocation(BillboardSettings.BillboardOffset);

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