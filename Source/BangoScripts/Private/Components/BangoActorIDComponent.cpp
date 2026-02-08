#include "BangoScripts/Components/BangoActorIDComponent.h"

#include "CanvasItem.h"
#include "TimerManager.h"
#include "BangoScripts/Subsystem/BangoActorIDSubsystem.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Engine/Canvas.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/ICookInfo.h"

#if WITH_EDITOR
#include "Editor.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#endif

UBangoActorIDComponent::UBangoActorIDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
#if WITH_EDITORONLY_DATA 
	if (!IsRunningCookCommandlet())
	{
		FCookLoadScope EditorOnlyLoadScope(ECookLoadType::EditorOnly);
		IconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Bango/Icon_ActorID.Icon_ActorID"));
	}
#endif
}

void UBangoActorIDComponent::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	
#if WITH_EDITOR
	// Manual names should normally only be assigned at edit-time; if duplicating at game time, don't use a name
	if (!Bango::Editor::IsComponentInEditedLevel(this))
	{
		BangoName = NAME_None;
	}
#endif
}

void UBangoActorIDComponent::BeginPlay()
{
	Super::BeginPlay();

	UBangoActorIDSubsystem::RegisterActor(this, GetOwner(), BangoName, BangoGuid);
}

void UBangoActorIDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UBangoActorIDSubsystem::UnregisterActor(this, BangoGuid);
	
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void UBangoActorIDComponent::PrintGuid(const FString& FuncName)
{
	if (!Bango::Editor::IsComponentInEditedLevel(this))
	{
		return;
	}
	
	FString PrintedFuncName = FuncName;
	
	while (PrintedFuncName.Len() < 24)
	{
		PrintedFuncName += " ";
	}
	
	FString Msg = PrintedFuncName + " Guid: [" + BangoGuid.ToString() + "]";
	Bango::Debug::PrintComponentState(this, Msg);
}
#endif

#if WITH_EDITOR
void UBangoActorIDComponent::SetBangoName(FName NewID)
{
	Modify();
	BangoName = NewID;
}
#endif

#if WITH_EDITOR
void UBangoActorIDComponent::OnRegister()
{
	Super::OnRegister();
	
#if WITH_EDITOR
	if (Bango::Editor::IsComponentInEditedLevel(this))
	{
		TWeakObjectPtr<UBangoActorIDComponent> WeakThis = this;
		
		auto DelayedRegister = FTimerDelegate::CreateLambda( [WeakThis] ()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			
			WeakThis->EnsureValidGuid();
		});
		
		GEditor->GetTimerManager()->SetTimerForNextTick(DelayedRegister);
	}
	//BangoDebugDraw_Register<UBangoActorIDComponent>(this);
#endif
	
}
#endif

#if WITH_EDITOR
void UBangoActorIDComponent::OnUnregister()
{
	//BangoDebugDraw_Unregister(this);
	
	Super::OnUnregister();
}
#endif

#if WITH_EDITOR
void UBangoActorIDComponent::EnsureValidGuid()
{
	if (Bango::Editor::IsComponentInEditedLevel(this))
	{
		if (!BangoGuid.IsValid() || BangoGuid == FGuid(0, 0, 0, 1))
		{
			UE_LOG(LogBango, Display, TEXT("EnsureValidGuid called - existing Guid: %s"), *BangoGuid.ToString());
			Modify();
			BangoGuid = FGuid::NewGuid();
			UE_LOG(LogBango, Verbose, TEXT("Setting ID Component GUID to: %s"), *BangoGuid.ToString());
		}
	}
}
#endif

#if WITH_EDITOR
void UBangoActorIDComponent::DebugDrawEditor(UCanvas* Canvas, FVector ScreenLocation, float Alpha) const
{
	UTexture* BackgroundTex = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture"));
	UFont* Font = GEngine->GetLargeFont();
	
	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D TextSize = FontMeasureService->Measure(BangoName.ToString(), Font->GetLegacySlateFontInfo());
	
	FLinearColor TagColor = Bango::Colors::White;
	TagColor.A *= Alpha;
	
	float IconRawSize = 32.0f;
	float IconPadding = 4.0f;
	float IconScale = 0.5f;
	float Padding = 4.0f;
	float Border = 2.0f;
	
	float IconSize = IconRawSize * IconScale;
	
	{
		// Background Border
		float X = ScreenLocation.X - 0.5f * TextSize.X - Padding - 0.5f * IconSize - 0.5f * IconPadding;
		float Y = ScreenLocation.Y - 0.5f * FMath::Max(TextSize.Y, IconSize) - Padding;
		float XL = TextSize.X + 2.0 * Padding + IconSize + IconPadding;
		float YL = FMath::Max(TextSize.Y, IconSize) + 2.0 * Padding;
		FVector4f UV(0.0f, 0.0f, 1.0f, 1.0f);
	
		Canvas->SetDrawColor(FColor(150, 150, 150, 150 * Alpha));
		Canvas->DrawTile(BackgroundTex, X - Border, Y - Border, XL + 2.0 * Border, YL + 2.0 * Border, UV.X, UV.Y, UV.Z, UV.Z);
		
		Canvas->SetDrawColor(FColor(20, 20, 20, 150 * Alpha));
		Canvas->DrawTile(BackgroundTex, X, Y, XL, YL, UV.X, UV.Y, UV.Z, UV.Z);
	}
	
	{
		// ID Text
		float X = ScreenLocation.X + 0.5f * IconSize + 0.5f * IconPadding;
		float Y = ScreenLocation.Y - 0.5f;
		
		FCanvasTextItem Label(FVector2D(X, Y), FText::FromName(BangoName), Font, TagColor);
		Label.bCentreX = true;
		Label.bCentreY = true;
		Canvas->SetDrawColor(TagColor.ToFColor(false));
		Canvas->DrawItem(Label);
	}
	
	{
		// ID Icon
		float X = ScreenLocation.X - 0.5f * TextSize.X - 0.5f * IconSize - 0.5f * IconPadding;
		float Y = ScreenLocation.Y - 0.5f * IconSize;
		
		FCanvasIcon Icon = UCanvas::MakeIcon(IconTexture, 0.0f, 0.0f, IconRawSize, IconRawSize);
		Canvas->SetDrawColor(TagColor.ToFColor(false));
		Canvas->DrawIcon(Icon, X, Y, IconScale);
	}
}

void UBangoActorIDComponent::DebugDrawGame(UCanvas* Canvas, FVector ScreenLocation, float Alpha) const
{
	
}

#endif

