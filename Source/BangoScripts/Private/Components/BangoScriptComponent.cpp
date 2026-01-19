#include "BangoScripts/Components/BangoScriptComponent.h"

#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Subsystem/BangoScriptSubsystem.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Components/BillboardComponent.h"
#include "Fonts/FontMeasure.h"
#include "UObject/ICookInfo.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_EDITOR
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#endif

#define LOCTEXT_NAMESPACE "BangoScripts"

// ----------------------------------------------

UBangoScriptComponent::UBangoScriptComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA 
	if (!IsRunningCookCommandlet())
	{
		FCookLoadScope EditorOnlyLoadScope(ECookLoadType::EditorOnly);
		IconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/BangoScripts/Icon_Script.Icon_Script"));
	}
#endif
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
		FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(this, &ScriptContainer, ScriptName);
	}
	
	if (!IsTemplate())
	{
		FBangoEditorDelegates::BangoDebugDraw.AddUObject(this, &ThisClass::DebugDraw);
	}
	
#if 0
	if (!Billboard && GetOwner() && !GetWorld()->IsGameWorld())
	{
		{
			FCookLoadScope EditorOnlyLoadScope(ECookLoadType::EditorOnly);
			const EObjectFlags TransactionalFlag = GetFlags() & RF_Transactional;
		
			Billboard = NewObject<UBillboardComponent>(GetOwner(), NAME_None, TransactionalFlag | RF_Transient | RF_TextExportTransient);
			Billboard->Sprite = IconTexture;
		}
		
		Billboard->SetupAttachment(GetOwner()->GetRootComponent());
		Billboard->bHiddenInGame = true;
		Billboard->bIsScreenSizeScaled = true;
		Billboard->SetRelativeLocation(100.0f * FVector::UpVector);
		
		
		Billboard->SetRelativeScale3D_Direct(FVector(1.0f, 1.0f, 1.0f));
		Billboard->Mobility = EComponentMobility::Movable;
		Billboard->AlwaysLoadOnClient = false;
		Billboard->SetIsVisualizationComponent(true);
		Billboard->SpriteInfo.Category = TEXT("Misc");
		Billboard->SpriteInfo.DisplayName = NSLOCTEXT("SpriteCategory", "Misc", "Misc");
		Billboard->CreationMethod = CreationMethod;
		Billboard->bIsScreenSizeScaled = true;
		Billboard->bUseInEditorScaling = true;
		Billboard->OpacityMaskRefVal = .3f;
		
		Billboard->RegisterComponent();
	}
#endif
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnUnregister()
{
	FBangoEditorDelegates::BangoDebugDraw.RemoveAll(this);

	FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(this, &ScriptContainer);
		
	Super::OnUnregister();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	
	if (!Bango::Editor::IsComponentInEditedLevel(this))
	{
		return;
	}
	
	// With RF_LoadCompleted this is a default actor component. We rely on PostDuplicated instead.
	if (HasAllFlags(RF_LoadCompleted))
	{
		return;
	}
	
	FString ScriptName = GetName(); // We will use the component name for the script name
	
	if (ScriptContainer.GetGuid().IsValid())
	{
		FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(this, &ScriptContainer, ScriptName);
	}
	else
	{
		FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(this, &ScriptContainer, ScriptName);
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	TSoftClassPtr<UBangoScript> ScriptClass = ScriptContainer.GetScriptClass();
	
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	
	if (!ScriptClass.IsNull())
	{
		// Moves handling over to an editor module to handle more complicated package deletion/undo management
		FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(this, &ScriptContainer);
	}
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
		FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(this, &ScriptContainer, GetName());
	}
	else
	{
		Bango::Debug::PrintComponentState(this, "PostDuplicate_CDO");

		// Component is part of a blueprint, only run duplication code if it's in a level already
		if (!GetOwner()->HasAnyFlags(RF_WasLoaded))
		{
			FBangoEditorDelegates::OnScriptContainerDuplicated.Broadcast(this, &ScriptContainer, GetName());
		}
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::UnsetScript()
{
	Modify();
	ScriptContainer.Unset();
	
	if (!MarkPackageDirty())
	{
		UE_LOG(LogBlueprint, Error, TEXT("Could not mark the actor package dirty?"));
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// ----------------------------------------------

void UBangoScriptComponent::Run()
{
#if WITH_EDITOR
	FBangoScriptHandle Handle =  
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
FGuid UBangoScriptComponent::GetScriptGuid() const
{
	return ScriptContainer.GetGuid();
}
#endif

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
void UBangoScriptComponent::DebugDraw(FBangoDebugDrawCanvas& Canvas, bool bPIE) const
{
	if (bPIE)
	{
		DebugDrawGame(Canvas);
	}
	else
	{
		DebugDrawEditor(Canvas);
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::DebugDrawEditor(FBangoDebugDrawCanvas& Canvas) const
{
	FLinearColor TagColor = Bango::Colors::White;

	float Alpha = Canvas.GetAlpha(GetOwner());
	
	if (Alpha <= 0.0f)
	{
		return;
	}
	
	FVector ScreenLocation = Canvas.GetNextScreenPos(GetOwner());
	
	if (ScreenLocation.Z <= 0.0f)
	{
		return;
	}
	
	if (GetWorld()->IsGameWorld())
	{
		if (RunningHandle.IsExpired())
		{
			TagColor = Bango::Colors::YellowBase;
		}
		else if (RunningHandle.IsRunning())
		{
			TagColor = Bango::Colors::LightBlue;
		}
		else if (RunningHandle.IsNull())
		{
			TagColor = FLinearColor::Black;
		}
	}
	else
	{
		if (bRunOnBeginPlay)
		{
			TagColor = Bango::Colors::Green;
		}
	}
	
	FText LabelText = FText::FromString(ScriptContainer.GetDescription());
	
	if (LabelText.IsEmpty())
	{
		LabelText = FText::FromString(GetName());
	}
	
	FVector2D TextSize = FVector2D::ZeroVector;
	UFont* Font = GEngine->GetLargeFont();
	
	TagColor.A *= Alpha;
	
	float IconRawSize = 32.0f;
	float IconScale = 0.5f;
	
	float IconSize = IconRawSize * IconScale;
	float Padding = 4.0f;
	float Border = 2.0f;
	float IconPadding = 4.0f;
	
	if (!LabelText.IsEmpty())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TextSize = FontMeasureService->Measure(LabelText.ToString(), Font->GetLegacySlateFontInfo());
	}

	float TotalWidth = IconSize;
	float TotalHeight = FMath::Max(IconSize, IconSize);
	
	if (TextSize.X > KINDA_SMALL_NUMBER)
	{
		TotalWidth += IconPadding + TextSize.X;
	}
	
	{
		// Background
		float X = ScreenLocation.X - 0.5f * TotalWidth - Padding;
		float Y = ScreenLocation.Y - 0.5f * TotalHeight - Padding;
		float XL = TotalWidth + 2.0f * Padding;
		float YL = TotalHeight + 2.0f * Padding;
		
		FVector4f UV(0.0f, 0.0f, 1.0f, 1.0f);
	
		UTexture* BackgroundTex = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture"));
		
		// Larger whitish rectangle
		Canvas->SetDrawColor(FColor(150, 150, 150, 150 * Alpha));
		Canvas->DrawTile(BackgroundTex, X - Border, Y - Border, XL + 2.0 * Border, YL + 2.0 * Border, UV.X, UV.Y, UV.Z, UV.Z);
		
		// Darker background
		Canvas->SetDrawColor(FColor(20, 20, 20, 150 * Alpha));
		Canvas->DrawTile(BackgroundTex, X, Y, XL, YL, UV.X, UV.Y, UV.Z, UV.Z);
	}
	
	{
		// ID Icon
		float X = ScreenLocation.X - 0.5f * TotalWidth;
		float Y = ScreenLocation.Y - 0.5f * IconSize;
		
		FCanvasIcon Icon = UCanvas::MakeIcon(IconTexture, 0.0f, 0.0f, IconRawSize, IconRawSize);
		Canvas->SetDrawColor(TagColor.ToFColor(false));
		Canvas->DrawIcon(Icon, X, Y, IconScale);
	}
	
	if (!LabelText.IsEmpty())
	{
		// Text
		float X = ScreenLocation.X - 0.5f * TotalWidth + 0.5f * IconSize + 0.5f * IconPadding;
		float Y = ScreenLocation.Y;
		
		FCanvasTextItem Text(FVector2D(X + 0.5f * IconSize + IconPadding, Y), LabelText, Font, Alpha * Bango::Colors::White);
		Text.bCentreY = true;
		Canvas->DrawItem(Text);
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptComponent::DebugDrawGame(FBangoDebugDrawCanvas& Canvas) const
{
	DebugDrawEditor(Canvas);
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

#undef LOCTEXT_NAMESPACE