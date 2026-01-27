#include "BangoDebugDraw_ScriptComponent.h"

#include "EditorViewportClient.h"
#include "LevelEditor.h"
#include "SceneView.h"
#include "SLevelViewport.h"
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "BlueprintEditor/BangoScriptBlueprintEditor.h"
#include "Modules/ModuleManager.h"
#include "Components/BillboardComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

FBangoDebugDraw_ScriptComponentHover UBangoDebugDraw_ScriptComponent::HoverInfo;

struct FBangoActorNodeDraw
{
	TSoftObjectPtr<const AActor> Actor = nullptr;
	bool bFocused = false;
	
	bool operator==(const FBangoActorNodeDraw& Other) const { return Other.Actor == this->Actor; }
	
	friend uint32 GetTypeHash(const FBangoActorNodeDraw& Struct)
	{
		return GetTypeHash(Struct.Actor);
	}
};

void FBangoDebugDraw_ScriptComponentHover::Reset()
{
	FocusedComponent.Reset();
	ScreenDistance = -1.0f;
	StartFocusTime = -1.0f;
		
	FSlateApplication::Get().DismissMenu(ActiveMenu);
	ActiveMenu = nullptr;
}

bool FBangoDebugDraw_ScriptComponentHover::Try(const UBangoScriptComponent* Contender, float MouseDistanceToBillboard)
{
	// Always succeed if we're the only contender
	if (!FocusedComponent.IsValid())
	{
		SwitchFocus(Contender);
		ScreenDistance = MouseDistanceToBillboard;
		return true;
	}

	// If this is the current contender, just update our mouse to billboard distance 
	if (FocusedComponent == Contender)
	{
		ScreenDistance = MouseDistanceToBillboard;
		return false;
	}
		
	// If this is a new contender with a better distance, select it
	if (MouseDistanceToBillboard < ScreenDistance)
	{
		if (FocusedComponent != Contender)
		{
			SwitchFocus(Contender);
			return true;
		}
	}

	return false;
}

void FBangoDebugDraw_ScriptComponentHover::SwitchFocus(const UBangoScriptComponent* NewFocus)
{
	FocusedComponent = NewFocus;
	StartFocusTime = NewFocus->GetWorld()->GetRealTimeSeconds();
}

UBangoDebugDraw_ScriptComponent::UBangoDebugDraw_ScriptComponent()
{
	if (IsTemplate())
	{
		UBangoScriptComponent::OnDebugDrawEditor.AddStatic(&ThisClass::DebugDrawEditor);
		UBangoScriptComponent::OnDebugDrawPIE.AddStatic(&ThisClass::DebugDrawPIE);
	}
}

void UBangoDebugDraw_ScriptComponent::DebugDrawEditor(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)
{
	if (!ScriptComponent)
	{
		return;
	}
	
	UBillboardComponent* ScriptBillboard = ScriptComponent->GetBillboard();
	
	if (!ScriptBillboard)
	{
		return;
	}
	
	FVector BillboardScreenPos;
	if (!Canvas.GetScreenLocation(ScriptBillboard, BillboardScreenPos))
	{
		return;
	}
	
	float Alpha = Canvas.GetAlpha(ScriptBillboard->GetComponentLocation(), false);
	if (Alpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	FIntPoint MousePoint;
	if (!Canvas.GetMousePosInLevelViewport(MousePoint))
	{
		return;
	}
	
	FVector MousePos(MousePoint.X, MousePoint.Y, 0.0f);
	BillboardScreenPos.Z = 0.0f;
	
	if (FVector::DistSquared(MousePos, BillboardScreenPos) > 2500.0f)
	{
		return;
	}
	
	DebugDrawEditorImpl(Canvas, ScriptComponent, Alpha, BillboardScreenPos);
}

void UBangoDebugDraw_ScriptComponent::DebugDrawEditorImpl(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent, float Alpha, const FVector& BillboardScreenPos)
{
	FLinearColor TagColor = Bango::Colors::White;

	const FBangoScriptHandle& RunningHandle = ScriptComponent->GetRunningHandle();
	
	if (ScriptComponent->GetWorld()->IsGameWorld())
	{
		if (RunningHandle.IsNull())
		{
			TagColor = FLinearColor::Black;
		}
	}
	else
	{
		if (ScriptComponent->GetRunOnBeginPlay())
		{
			TagColor = Bango::Colors::Green;
		}
	}
	
	FText LabelText = FText::FromString(ScriptComponent->GetScriptContainer().GetDescription());
	
	if (LabelText.IsEmpty())
	{
		if (!ScriptComponent->GetScriptContainer().GetScriptClass().IsNull())
		{
			LabelText = FText::FromString(ScriptComponent->GetName());
		}
		else
		{
			LabelText = LOCTEXT("DebugDrawPopup_NoScript", "No script"); 
		}
	}
	
	FVector2D TextSize = FVector2D::ZeroVector;
	UFont* Font = GEngine->GetLargeFont();
	
	TagColor.A *= Alpha;
	
	float Padding = 4.0f;
	float Border = 2.0f;
	// float IconPadding = 4.0f;
	
	if (!LabelText.IsEmpty())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TextSize = FontMeasureService->Measure(LabelText.ToString(), Font->GetLegacySlateFontInfo());
	}

	float TotalWidth = 0;
	float TotalHeight = 16;
	
	if (TextSize.X > KINDA_SMALL_NUMBER)
	{
		TotalWidth += TextSize.X;
	}
	
    float WidgetCenterX = BillboardScreenPos.X;
    float WidgetCenterY = BillboardScreenPos.Y;// + 2.0f * TotalHeight;
    
    float RightOffset = 25.0f;
    
	{
		// Background
		float X = WidgetCenterX + RightOffset; // - 0.5f * TotalWidth - Padding;
		float Y = WidgetCenterY - 0.5f * TotalHeight - Padding;
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
	
	if (!LabelText.IsEmpty())
	{
		// Text
		float X = WidgetCenterX + RightOffset + Padding;// + 0.5f * TotalWidth;
		float Y = WidgetCenterY;
		
		FCanvasTextItem Text(FVector2D(X, Y), LabelText, Font, Alpha * Bango::Colors::White);
        //Text.bCentreX = true;
	    Text.bCentreY = true;
		Canvas->DrawItem(Text);
	}
}

void UBangoDebugDraw_ScriptComponent::DebugDrawPIE(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent)
{
	if (!ScriptComponent)
	{
		return;
	}
	
    AActor* Owner = ScriptComponent->GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }
    
    FVector AnchorPosition = Owner->GetActorLocation() + ScriptComponent->GetBillboardOffset();
	
    const FConvexVolume& Frustum = Canvas->SceneView->GetCullingFrustum();
    if (!Frustum.IntersectPoint(AnchorPosition))
    {
        return;
    }
    
	FVector BillboardScreenPos;
	if (!Canvas.GetScreenLocation(AnchorPosition, BillboardScreenPos))
	{
		return;
	}
	
	float Alpha = Canvas.GetAlpha(BillboardScreenPos, true);
	if (Alpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	BillboardScreenPos.Z = 0.0f;
	
	// ------------------------------------------
	// Check if mouse is over this component
	FIntPoint MousePoint;
	bool bMouseOver = false;
	float MouseDistSqrd = -1;
	if (Canvas.GetMousePosInLevelViewport(MousePoint))
	{
		FVector MousePos(MousePoint.X, MousePoint.Y, 0.0f);
		MouseDistSqrd = FVector::DistSquared(MousePos, BillboardScreenPos);
	}
	
	DebugDrawPIEImpl(Canvas, ScriptComponent, Alpha, MouseDistSqrd, BillboardScreenPos);
	
	FWorldContext* Context = GEngine->GetWorldContextFromWorld(ScriptComponent->GetWorld());

	// NOTE: bIsSimulatingInEditor is SUPPOSED to be deprecated since 4.25 but the "Eject" button still uses it.
	//if (GEditor->IsSimulateInEditorInProgress() || GEditor->bIsSimulatingInEditor)

	if (Context && Context->GameViewport && Context->GameViewport->IsSimulateInEditorViewport())
	{
		DrawRunScriptInPIEWidget(Canvas, ScriptComponent, Alpha, MouseDistSqrd, BillboardScreenPos);
	}
}

void UBangoDebugDraw_ScriptComponent::DebugDrawPIEImpl(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos)
{
	FLinearColor TagColor = Bango::Colors::White;
	
	int32 U = 0;
	int32 V = 0;
	int32 UL = 64;
	int32 VL = 64;
		
	if (ScriptComponent->GetScriptContainer().GetScriptClass().IsNull())
	{
		return;
	}
	
	const FBangoScriptHandle& RunningHandle = ScriptComponent->GetRunningHandle();
	
	if (RunningHandle.IsRunning())
	{
		U = 0;
		V = 64;
	}
	else if (RunningHandle.IsExpired())
	{
		U = 64;
		V = 64;
	}

	TagColor.A *= Alpha;
	
	float IconRawSize = 64.0f;
	float IconScale = 0.5f;
	float IconSize = IconRawSize * IconScale;
	
	{
		// Script Icon
		float X = BillboardScreenPos.X - 0.5f * IconSize;
		float Y = BillboardScreenPos.Y - 0.5f * IconSize;
		
		FCanvasIcon Icon = UCanvas::MakeIcon(Bango::Debug::GetScriptPIESprite(), U, V, UL, VL);
		Canvas->SetDrawColor(TagColor.ToFColor(false));
		Canvas->DrawIcon(Icon, X, Y, IconScale);
	}
}

void UBangoDebugDraw_ScriptComponent::DrawRunScriptInPIEWidget(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos)
{
	static TSharedPtr<SWidget> PopupWidget = nullptr;
	const float MouseThreshold = 2500.0f;
	
	if (MouseDistSqrd < MouseThreshold)
	{
		// We are hovering over this billboard! If it was not focused, *try* to focus on it. Focus will fail if it is already focused by something closer to the mouse.
		if (HoverInfo.Try(ScriptComponent, MouseDistSqrd))
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
			TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
			if (LevelEditor.IsValid())
			{
				TSharedPtr<SLevelViewport> LevelViewport = LevelEditor->GetActiveViewportInterface();
					
				if (LevelViewport)
				{
					TWeakObjectPtr<UBangoScriptComponent> WeakScriptComponent = ScriptComponent;
					
					TSharedRef<SWidget> HoverWidget = SNew(SButton)
						.Text(INVTEXT("Run"))
						.OnClicked_Lambda([WeakScriptComponent]()
						{
							if (WeakScriptComponent.IsValid())
							{
								WeakScriptComponent->Run();
							}
							
							return FReply::Handled();
						});
					
					FVector2f ViewportPosition = LevelViewport->GetCachedGeometry().GetAbsolutePosition();
					FVector2f BillboardAbsPos = ViewportPosition + FVector2f(BillboardScreenPos.X, BillboardScreenPos.Y);
					
					HoverInfo.ActiveMenu = FSlateApplication::Get().PushMenu(
						LevelViewport.ToSharedRef(),
						FWidgetPath(),
						HoverWidget,
						BillboardAbsPos,
						FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup), 
						false);
				}
			}
		}
	}
	else
	{
		// We are NOT hovering over this billboard. If it was focused, unfocus it.
		if (HoverInfo.FocusedComponent == ScriptComponent)
		{
			HoverInfo.Reset();
			return;
		}
	}
}

#undef LOCTEXT_NAMESPACE
