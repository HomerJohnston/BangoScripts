#include "BangoDebugDraw_ScriptComponent.h"

#if 0

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

FText UBangoDebugDraw_ScriptComponent::GetLabelText(const UBangoScriptComponent& ScriptComponent)
{
	FText LabelText = FText::FromString(ScriptComponent.GetScriptContainer().GetDescription());
	
	if (LabelText.IsEmpty())
	{
		if (!ScriptComponent.GetScriptContainer().GetScriptClass().IsNull())
		{
			LabelText = FText::FromString(ScriptComponent.GetName());
		}
		else
		{
			LabelText = LOCTEXT("DebugDrawPopup_NoScript", "No script"); 
		}
	}
	
	return LabelText;
}

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
    if (FocusedComponent.IsValid())
    {
	    FocusedComponent.Reset();
	    ScreenDistance = -1.0f;
	    StartFocusTime = -1.0f;
		
	    FSlateApplication::Get().DismissMenu(ActiveMenu);
	    ActiveMenu = nullptr;
	    
        if (bSlateThrottle)
        {
	        FSlateThrottleManager::Get().DisableThrottle(false);
            bSlateThrottle = false;    
        }
    }
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

    if (!bSlateThrottle)
    {
        FSlateThrottleManager::Get().DisableThrottle(true);
        bSlateThrottle = true;    
    }
}

UBangoDebugDraw_ScriptComponent::UBangoDebugDraw_ScriptComponent()
{
	if (IsTemplate())
	{
		UBangoScriptComponent::OnDebugDrawPIE.AddStatic(&ThisClass::DebugDrawPIE);
	}
}

/*
void UBangoDebugDraw_ScriptComponent::DebugDrawEditor(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent)
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
	
	BillboardScreenPos.Z = 0.0f;
	FVector2D BillboardScreenPos2D(BillboardScreenPos);
	
	// ------------------------------------------
	// Check if mouse is over this component

	float MouseDistSqrd;
	
	FViewport* Viewport = GEditor->GetActiveViewport();
	
	if (Viewport)
	{
		// When the viewport is focused, we only draw extra debug widgets in Simulate mode
		if (Canvas.GetMousePosInLevelViewport(MousePoint))
		{
			FVector2D MousePos(MousePoint.X, MousePoint.Y);
			MouseDistSqrd = FVector2D::DistSquared(MousePos, BillboardScreenPos2D);
	
			DrawViewportHoverControls(ScriptComponent, Alpha, MouseDistSqrd, BillboardScreenPos, false);
			return;				    
		}
	}

	// TODO if a hover is active, and somehow the user turns off the debug showflag, this won't execute anymore we we could be left with a rogue popup?
	// ^^^ This should be impossible but... who knows. If it can break, it will.
	
	// Always make sure we don't get left with a rogue popup
	if (HoverInfo.FocusedComponent == ScriptComponent)
	{
		HoverInfo.Reset();
	}
}
*/


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
	FVector2D BillboardScreenPos2D(BillboardScreenPos);
	
	// ------------------------------------------
	// Check if mouse is over this component

	FIntPoint MousePoint;
	float MouseDistSqrd = -1;
	
	DebugDrawPIEImpl(Canvas, ScriptComponent, Alpha, MouseDistSqrd, BillboardScreenPos);
	FWorldContext* Context = GEngine->GetWorldContextFromWorld(ScriptComponent->GetWorld());
	UGameViewportClient* GameViewport = Context ? Context->GameViewport : nullptr;
	
	if (GameViewport)
	{
		TSharedPtr<SViewport> ViewportWidget = GameViewport->GetGameViewportWidget();
		
		if (ViewportWidget)
		{
			if (GameViewport->GetGameViewportWidget()->HasAnyUserFocus())
			{
				// When the viewport is focused, we only draw extra debug widgets in Simulate mode
				if (GameViewport->IsSimulateInEditorViewport() && Canvas.GetMousePosInLevelViewport(MousePoint))
				{
					FVector2D MousePos(MousePoint.X, MousePoint.Y);
					MouseDistSqrd = FVector2D::DistSquared(MousePos, BillboardScreenPos2D);
			
					DrawViewportHoverControls(ScriptComponent, Alpha, MouseDistSqrd, BillboardScreenPos, true);
			        return;				    
				}
			}
			else
			{
				// Shift+F1 was pressed, viewport is not focused
				FVector2D GlobalMousePos = FSlateApplication::Get().GetCursorPos();
				FVector2D RelativeMousePos = GlobalMousePos - ViewportWidget->GetCachedGeometry().GetAbsolutePosition();
				
				MouseDistSqrd = FVector2D::DistSquared(RelativeMousePos, BillboardScreenPos2D);
				
				DrawViewportHoverControls(ScriptComponent, Alpha, MouseDistSqrd, BillboardScreenPos, true);
			    return;
			}
		}
	}

	// TODO if a hover is active, and somehow the user turns off the debug showflag, this won't execute anymore we we could be left with a rogue popup?
	// ^^^ This should be impossible but... who knows. If it can break, it will.
	
    // Always make sure we don't get left with a rogue popup
    if (HoverInfo.FocusedComponent == ScriptComponent)
    {
	    HoverInfo.Reset();
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

void UBangoDebugDraw_ScriptComponent::DrawViewportHoverControls(UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos, bool bPIE)
{
	if (!IsValid(ScriptComponent))
	{
		return;
	}
	
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
					
					FText LabelText = GetLabelText(*ScriptComponent);
					
					TSharedRef<SVerticalBox> HoverWidgetVerticalBox = SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2, 2, 2, 2)
					[
						SNew(STextBlock)
						.Text(LabelText)
						.TextStyle(FAppStyle::Get(), "SmallText")
					]
				    + SVerticalBox::Slot()
				    .AutoHeight()
				    .Padding(2, 2, 2, 2)
				    [
                        SNew(SButton)
                        .Text(INVTEXT("Edit"))
						.TextStyle(FAppStyle::Get(), "SmallText")
                        .OnClicked_Lambda([WeakScriptComponent]()
                        {
                            if (WeakScriptComponent.IsValid())
                            {
                                UBangoScriptBlueprint* Blueprint = WeakScriptComponent->GetScriptBlueprint(true);
                                if (Blueprint)
                                {
    			                    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Blueprint);
                                }
                            }
                            return FReply::Handled();
                        })
				    ];
					
					if (bPIE)
					{
						HoverWidgetVerticalBox->AddSlot()
						.Padding(2, 2, 2, 2)
						.AutoHeight()
						[
							SNew(SButton)
							.Text(INVTEXT("Run"))
							.TextStyle(FAppStyle::Get(), "SmallText")
							.OnClicked_Lambda([WeakScriptComponent]()
							{
								if (WeakScriptComponent.IsValid())
								{
									WeakScriptComponent->Run();
								}
								return FReply::Handled();
							})
						];
					}

					FVector2f ViewportPosition = LevelViewport->GetCachedGeometry().GetAbsolutePosition();
					FVector2f BillboardAbsPos = ViewportPosition + FVector2f(BillboardScreenPos.X, BillboardScreenPos.Y);
					
					HoverInfo.ActiveMenu = FSlateApplication::Get().PushMenu(
						LevelViewport.ToSharedRef(),
						FWidgetPath(),
						HoverWidgetVerticalBox,
						BillboardAbsPos,
						FPopupTransitionEffect(FPopupTransitionEffect::SubMenu), 
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

#endif