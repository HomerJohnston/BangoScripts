#include "BangoScriptsDebugDrawService.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "IHeadMountedDisplay.h"
#include "IXRTrackingSystem.h"
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "SceneView.h"
#include "SLevelViewport.h"
#include "Application/ThrottleManager.h"
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoDebugDrawCanvas.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "BangoScripts_EditorTooling/BangoScripts_EditorTooling.h"
#include "Components/Viewport.h"
#include "Debug/DebugDrawService.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ================================================================================================

FBangoScriptOctreeElement::FBangoScriptOctreeElement(UBangoScriptComponent* InScriptComponent)
{
	check(IsValid(InScriptComponent));
	ScriptComponent = InScriptComponent;

	Update();
}

// ----------------------------------------------

bool FBangoScriptOctreeElement::Update()
{
	FVector OldPosition = Position;
	Position = ScriptComponent->GetBillboardPosition();
	
	return FVector::DistSquared(OldPosition, Position) > KINDA_SMALL_NUMBER;
}

// ----------------------------------------------

void FBangoScriptOctreeSemantics::SetElementId(const FBangoScriptOctreeElement& Element, FOctreeElementId2 Id)
{
	if (Element.ScriptComponent.IsValid())
	{
		Element.ScriptComponent->DebugElementId = Id;
	}
}

// ================================================================================================

bool FBangoDebugDraw_ScriptComponentHover::TryToFocusOnComponent(const UBangoScriptComponent* Contender, float MouseDistanceToBillboard)
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

// ----------------------------------------------

TWeakObjectPtr<const UBangoScriptComponent> FBangoDebugDraw_ScriptComponentHover::GetFocusedComponent() const
{
	return FocusedComponent;
}

// ----------------------------------------------

void FBangoDebugDraw_ScriptComponentHover::SetActiveMenuAndWidget(TSharedPtr<SWidget> InMenuWidget, TSharedPtr<IMenu> InMenu)
{
	ActiveMenuWidget = InMenuWidget;
	ActiveMenu = InMenu;
}

// ----------------------------------------------

void FBangoDebugDraw_ScriptComponentHover::Reset()
{
	FocusedComponent.Reset();
	ScreenDistance = -1.0f;
	StartFocusTime = -1.0f;
	
	FSlateApplication::Get().DismissMenu(ActiveMenu);
	ActiveMenu = nullptr;
    ActiveMenuWidget = nullptr;
	
	if (bSlateThrottle)
	{
		FSlateThrottleManager::Get().DisableThrottle(false);
		bSlateThrottle = false;    
	}
}

// ----------------------------------------------

bool FBangoDebugDraw_ScriptComponentHover::IsHoveredOrHoverPendingFor(UBangoScriptComponent* ScriptComponent) const
{
	return FocusedComponent == ScriptComponent && !bHoverConsumed;
}

// ----------------------------------------------

bool FBangoDebugDraw_ScriptComponentHover::HasFocusedComponent() const
{
	return FocusedComponent.IsValid();
}

// ----------------------------------------------

bool FBangoDebugDraw_ScriptComponentHover::IsWidgetVisibleAndHovered() const
{	
	if (!ActiveMenuWidget.IsValid())
	{
		return false;
	}
	
	const FVector2D AbsoluteMousePos = FSlateApplication::Get().GetCursorPos();
	
	return ActiveMenuWidget->GetCachedGeometry().IsUnderLocation(AbsoluteMousePos);
}

// ----------------------------------------------

bool FBangoDebugDraw_ScriptComponentHover::ConsumeHovered()
{
	const float HoverTime = 0.05f;
	
	if (!bHoverConsumed && FocusedComponent.IsValid() && (FocusedComponent->GetWorld()->GetRealTimeSeconds() - StartFocusTime) >= HoverTime)
	{
		bHoverConsumed = true;
		return true;
	}
	
	return false;
}

// ----------------------------------------------

void FBangoDebugDraw_ScriptComponentHover::SwitchFocus(const UBangoScriptComponent* NewFocus)
{
	FocusedComponent = NewFocus;
	StartFocusTime = NewFocus->GetWorld()->GetRealTimeSeconds();
	bHoverConsumed = false;

	if (!bSlateThrottle)
	{
		FSlateThrottleManager::Get().DisableThrottle(true);
		bSlateThrottle = true;    
	}
}

// ================================================================================================

void UBangoScriptsDebugDrawService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	DebugDrawHandle = UDebugDrawService::Register(TEXT("BangoScriptsShowFlag"), FDebugDrawDelegate::CreateUObject(this, &ThisClass::DebugDraw));
	FBangoEditorDelegates::ScriptComponentRegistered.AddUObject(this, &ThisClass::OnBangoScriptRegistrationChange);
	
	// TODO this should not be needed! I should be able to subscribe to TransformUpdated of the actor's root component, but I can't. See header for more notes.
	GEngine->OnActorMoved().AddUObject(this, &ThisClass::OnGlobalActorMoved);
	
	FEditorDelegates::OnMapLoad.AddWeakLambda(this, [this] (const FString&, FCanLoadMap&) { ScriptOwners.Empty(); ScriptComponentTree.Destroy(); } );
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::Deinitialize()
{
	UDebugDrawService::Unregister(DebugDrawHandle);
	FBangoEditorDelegates::ScriptComponentRegistered.RemoveAll(this);
	
	// TODO this should not be needed! I should be able to subscribe to TransformUpdated of the actor's root component, but I can't. See header for more notes.
	GEngine->OnActorMoved().RemoveAll(this);
	
	Super::Deinitialize();
}

// ----------------------------------------------

bool UBangoScriptsDebugDrawService::IsTickable() const
{
	return !IsTemplate();
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::DebugDraw(UCanvas* Canvas, APlayerController* ALWAYSNULL_DONOTUSE)
{
	if (GEditor->IsPlaySessionInProgress())
	{
		UpdateNearbyScripts();
		
		for (FBangoScripts_NearbyScript& NearbyScript : NearbyScripts)
		{
			const UBangoScriptComponent* ScriptComponent = NearbyScript.Component;
		
			DrawPIEIcon(Canvas, ScriptComponent, NearbyScript.ScreenPos);
		}
	}
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::Tick(float DeltaTime)
{	
	if (!GEditor->IsPlaySessionInProgress())
	{
		UpdateNearbyScripts();
	}
	
	FViewport* Viewport = GEditor->GetActiveViewport();
	
	if (!Viewport || Viewport->GetSizeXY().X < 50 || Viewport->GetSizeXY().Y < 50)
	{
		return;
	} 
			
	FViewportClient* ViewportClient = Viewport->GetClient();
	FEngineShowFlags* ShowFlags = ViewportClient->GetEngineShowFlags();
	
	if (!FBangoScripts_EditorToolingModule::BangoScriptsShowFlag.IsEnabled(*ShowFlags))
	{
		NearbyScripts.Empty();
		return;
	}
	
	bool bFoundAnyHover = false;
	
	for (FBangoScripts_NearbyScript& NearbyScript : NearbyScripts)
	{
		UBangoScriptComponent* ScriptComponent = NearbyScript.Component;
		
		//DrawDebugSphere(ScriptComponent->GetWorld(), ScriptComponent->GetBillboardPosition(), 25.0f, 12, FColor::Cyan, false);
		
		bool bPIE = GEditor->IsPlaySessionInProgress();
		
		bFoundAnyHover |= DrawViewportHoverControls(ScriptComponent, NearbyScript.MouseDistSqrd, NearbyScript.ScreenPos, bPIE);
	}
	
	if (!bFoundAnyHover)
	{
		HoverInfo.Reset();
	}
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::UpdateNearbyScripts()
{
	static uint64 LastUpdateFrame = 0;
	
	if (LastUpdateFrame == GFrameCounter)
	{
		return;
	}
	
	LastUpdateFrame = GFrameCounter;
	
	NearbyScripts.Empty();
	
	FViewport* Viewport = GEditor->GetActiveViewport();
	
	if (!Viewport || Viewport->GetSizeXY().X < 50 || Viewport->GetSizeXY().Y < 50)
	{
		return;
	} 
			
	FViewportClient* ViewportClient = Viewport->GetClient();
	UWorld* World = ViewportClient->GetWorld();
	
	if (!FBangoScripts_EditorToolingModule::BangoScriptsShowFlag.IsEnabled(*ViewportClient->GetEngineShowFlags()))
	{
		return;
	}
	
	// TODO return if the camera is moving or other input is going on
	
	if (World->IsPlayInEditor())
	{
		UGameViewportClient* GameViewportClient = GEditor->GameViewport;
	
		if (!GameViewportClient)
		{
			return;
		}
		
		TSharedPtr<SViewport> ViewportWidget = GameViewportClient->GetGameViewportWidget();
		
		if (!ViewportWidget.IsValid())
		{
			return;
		}

		FVector2D GlobalMousePos = FSlateApplication::Get().GetCursorPos();
		
		// Check if mouse is within viewport
		const FVector2D AbsoluteMousePos = FSlateApplication::Get().GetCursorPos();
		if (!GameViewportClient->GetGameViewportWidget()->GetCachedGeometry().IsUnderLocation(AbsoluteMousePos))
		{
			return;
		}
		
		FVector2f MouseScreenPos = GlobalMousePos - ViewportWidget->GetCachedGeometry().GetAbsolutePosition();
		
		APlayerController* PlayerController = nullptr;
		
		for (APlayerController* Actor : TActorRange<APlayerController>(World))
		{
			PlayerController = Actor;
			break;
		}
		
		if (!PlayerController)
		{
			return;
		}

		AActor* ViewTarget = PlayerController->PlayerCameraManager->GetViewTarget();
		
		FMatrix One;
		FMatrix Two;
		FMatrix Three;
		UGameplayStatics::CalculateViewProjectionMatricesFromViewTarget(ViewTarget, One, Two, Three);
		
		FConvexVolume CullingFrustum;
		GetViewFrustumBounds(CullingFrustum, Three, true, true);
		
		auto FrustumTest = [this, CullingFrustum, PlayerController, MouseScreenPos](const FBangoScriptOctreeElement& ScriptElement)
		{
			if (ScriptElement.ScriptComponent.IsValid() && CullingFrustum.IntersectPoint(ScriptElement.Position) && ScriptElement.ScriptComponent->HasValidScript())
			{
				FVector2D ScriptScreenPos;
				
				if (PlayerController->ProjectWorldLocationToScreen(ScriptElement.Position, ScriptScreenPos))
				{
					float DistSqrd = FVector2f::DistSquared(FVector2f(ScriptScreenPos), MouseScreenPos);
			
					FBangoScripts_NearbyScript NearbyScript(ScriptElement.ScriptComponent.Get(), DistSqrd, FVector2f(ScriptScreenPos));
			
					NearbyScripts.Emplace(NearbyScript);	
				}
			}
		};
		
		FVector CameraPos = PlayerController->PlayerCameraManager->GetCameraLocation();
		
		FBoxCenterAndExtent CameraBox(CameraPos, FVector(5000.0f));
	
		ScriptComponentTree.FindElementsWithBoundsTest(CameraBox, FrustumTest);
	}
	else
	{
		FEditorViewportClient* EditorViewportClient = static_cast<FEditorViewportClient*>(ViewportClient);
	
		// Check if mouse is within viewport
		const FVector2D AbsoluteMousePos = FSlateApplication::Get().GetCursorPos();
		if (!EditorViewportClient->GetEditorViewportWidget()->GetCachedGeometry().IsUnderLocation(AbsoluteMousePos))
		{
			return;
		}
		
		// Check if script component is within camera frustum 
		FIntPoint MousePos;
		Viewport->GetMousePos(MousePos);
		FVector2f MouseScreenPos(MousePos.X, MousePos.Y);
		
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(EditorViewportClient->Viewport, EditorViewportClient->GetScene(), EditorViewportClient->EngineShowFlags));
		const FSceneView* SceneView = EditorViewportClient->CalcSceneView(&ViewFamily);
	
		FVector CameraPos = SceneView->ViewMatrices.GetViewOrigin();
		FConvexVolume CullingFrustum = SceneView->GetCullingFrustum();
		
		auto FrustumTest = [this, CullingFrustum, SceneView, MouseScreenPos](const FBangoScriptOctreeElement& ScriptElement)
		{
			if (ScriptElement.ScriptComponent.IsValid() && CullingFrustum.IntersectPoint(ScriptElement.Position) /*&& ScriptElement.ScriptComponent->HasValidScript()*/)
			{
				FVector2D PixelLocation;
				SceneView->WorldToPixel(ScriptElement.Position, PixelLocation);

				float DistSqrd = FVector2f::DistSquared(FVector2f(PixelLocation), MouseScreenPos);
			
				FBangoScripts_NearbyScript NearbyScript(ScriptElement.ScriptComponent.Get(), DistSqrd, FVector2f(PixelLocation));
			
				NearbyScripts.Emplace(NearbyScript);
			}
		};
	
		FBoxCenterAndExtent CameraBox(CameraPos, FVector(5000.0f));
	
		ScriptComponentTree.FindElementsWithBoundsTest(CameraBox, FrustumTest);
	}
}

// ----------------------------------------------

TStatId UBangoScriptsDebugDrawService::GetStatId() const
{
	return TStatId();
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::OnBangoScriptRegistrationChange(UBangoScriptComponent* ScriptComponent, EBangoScriptComponentRegisterStatus RegistrationStatus)
{
	FBangoScriptOctreeElement Element(ScriptComponent);
	
	if (RegistrationStatus == EBangoScriptComponentRegisterStatus::Registered)
	{
		AddElement(Element);
		// TODO this doesn't work. When dragging actor via gizmo handles, it does not fire PostEditChangeProperty of the root component
		ScriptComponent->GetOwner()->GetRootComponent()->TransformUpdated.AddUObject(this, &ThisClass::OnScriptComponentMoved, TWeakObjectPtr<UBangoScriptComponent>(ScriptComponent));
		ScriptOwners.Add(ScriptComponent->GetOwner());
	}
	else
	{
		ScriptOwners.Remove(ScriptComponent->GetOwner());
		ScriptComponent->GetOwner()->GetRootComponent()->TransformUpdated.RemoveAll(this);
		RemoveElement(Element);
	}
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::OnScriptComponentMoved(USceneComponent* SceneComponent,	EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport, TWeakObjectPtr<UBangoScriptComponent> ScriptComponent)
{
	if (ScriptComponent.Pin())
	{
		FBangoScriptOctreeElement Element(ScriptComponent.Get());
	
		//DrawDebugSphere(Element.ScriptComponent->GetWorld(), SceneComponent->GetComponentLocation(), 30.0f, 12, FColor::Yellow, false, 1.0f);
		
		RemoveElement(Element);
		AddElement(Element);
	}
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::AddElement(const FBangoScriptOctreeElement& Element)
{
	ScriptComponentTree.AddElement(Element);
	//DrawDebugSphere(Element.ScriptComponent->GetWorld(), Element.Position, 50.0f, 12, FColor::Green, false, 1.0f);
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::RemoveElement(const FBangoScriptOctreeElement& Element)
{
	//DrawDebugSphere(Element.ScriptComponent->GetWorld(), Element.Position, 50.0f, 12, FColor::Red, false, 1.0f);
	ScriptComponentTree.RemoveElement(Element.ScriptComponent.GetEvenIfUnreachable()->DebugElementId);
}

// ----------------------------------------------

void UBangoScriptsDebugDrawService::OnGlobalActorMoved(AActor* Actor)
{
	if (ScriptOwners.Contains(Actor))
	{
		TArray<UBangoScriptComponent*> ActorScriptComponents;
		Actor->GetComponents<UBangoScriptComponent>(ActorScriptComponents);
		
		for (UBangoScriptComponent* ScriptComponent : ActorScriptComponents)
		{
			FBangoScriptOctreeElement Element(ScriptComponent);
			
			RemoveElement(Element);
			AddElement(Element);
		}
	}
}

// ----------------------------------------------

FText UBangoScriptsDebugDrawService::GetLabelText(const UBangoScriptComponent& ScriptComponent)
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

// ----------------------------------------------


bool UBangoScriptsDebugDrawService::DrawViewportHoverControls(UBangoScriptComponent* ScriptComponent, float MouseDistSqrd, const FVector2f& BillboardScreenPos, bool bPIE)
{
	if (!IsValid(ScriptComponent))
	{
		return false;
	}
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (!LevelEditor)
	{
		return false;
	}
	
	TSharedPtr<SLevelViewport> LevelViewport = LevelEditor->GetActiveViewportInterface();
	if (!LevelViewport.IsValid())
	{
		return false;
	}

	TWeakPtr<SViewport> Viewport = LevelViewport->GetViewportWidget();
	if (!Viewport.IsValid())
	{
		return false;
	}
	
	// Don't try to display popup if we're actively playing
	if (bPIE && LevelViewport->HasAnyUserFocus())
	{
		return false;
	}
	
	// Don't try to display popup if this actor or component are selected
	if (ScriptComponent->GetOwner()->IsSelectedInEditor() || ScriptComponent->IsSelectedInEditor())
	{
		return false;
	}
	
	// TODO turn these into dev settings
	const float UnfocusedThreshold = 625.0f; // Squared screen distance (25 pixels)
	const float FocusedThreshold = 900.0f; // Squared screen distance (30 pixels)
	const float MouseThreshold = (HoverInfo.GetFocusedComponent() == ScriptComponent) ? FocusedThreshold : UnfocusedThreshold;
	
	// Don't try to display popup if the mouse is nowhere near the component; only evaluate this if we don't already have a popup
	if (!HoverInfo.IsWidgetVisibleAndHovered() && MouseDistSqrd > MouseThreshold)
	{
		return false;
	}
	
	// At this point, we know the mouse is near the evaluating component. Let's see if it can take over focus...
	HoverInfo.TryToFocusOnComponent(ScriptComponent, MouseDistSqrd);
	
	// Did we take over focus? If not, we're done
	if (HoverInfo.GetFocusedComponent() != ScriptComponent)
	{
		return false;
	}
	
	// We took over focus! See if hover time has elapsed to build the popup. This is "latching" so it will only run once.
	if (HoverInfo.ConsumeHovered())
	{
		TSharedRef<SWidget> MenuWidget = GetHoverMenuWidget(ScriptComponent, bPIE);
		MenuWidget->SlatePrepass();

		FVector2D WidgetSize = MenuWidget->GetDesiredSize();
		
		// The widget draw size needs to be multiplied by DPI to obtain the size the widget would have on screen.
		const TSharedPtr<SWindow> TopLevelWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		const float ScaleDPI = TopLevelWindow ? TopLevelWindow->GetDPIScaleFactor() : 1.f;
		WidgetSize *= ScaleDPI;
		
		FVector2f ViewportPosition = Viewport.Pin()->GetCachedGeometry().GetAbsolutePosition();
		FVector2f BillboardAbsPos = ViewportPosition + FVector2f(BillboardScreenPos.X - 0.5f * WidgetSize.X, BillboardScreenPos.Y - 0.5f * WidgetSize.Y);
		
		TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
			LevelViewport.ToSharedRef(),
			FWidgetPath(),
			MenuWidget,
			//MousePos, 
			BillboardAbsPos,
			FPopupTransitionEffect(FPopupTransitionEffect::None), 
			false);
		
		HoverInfo.SetActiveMenuAndWidget(MenuWidget, Menu);
	}
	
	return true;
}

TSharedRef<SWidget> UBangoScriptsDebugDrawService::GetHoverMenuWidget(UBangoScriptComponent* ScriptComponent, bool bPIE)
{
	if (!IsValid(ScriptComponent))
	{
		return SNew(STextBlock)
			.Text(INVTEXT("ERROR"));
	}
	
	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);
	TSharedPtr<SWidget> Title;
	TSharedPtr<SWidget> EditButton;
	TSharedPtr<SWidget> NewButton;
	TSharedPtr<SWidget> RunButton;
	
	if (ScriptComponent->HasValidScript())
	{
		Title = CreatePopupTitle(*ScriptComponent);
		EditButton = CreateEditButton(*ScriptComponent);
	
		if (bPIE)
		{
			RunButton = CreateRunButton(*ScriptComponent);
		}
	}
	else
	{
		NewButton = CreateNewButton(*ScriptComponent);
	}
	
	auto AddRow = [VerticalBox] (TSharedPtr<SWidget> RowWidget)
	{
		if (!RowWidget)
		{
			return;
		}
		
		VerticalBox->AddSlot()
		.AutoHeight()
		.Padding(2)
		[
			RowWidget.ToSharedRef()
		];
	};
	
	AddRow(Title);
	AddRow(EditButton);
	AddRow(NewButton);
	AddRow(RunButton);
	
	return SNew(SBox)
	.Padding(2)
	[
		VerticalBox
	];
}

void UBangoScriptsDebugDrawService::DrawPIEIcon(UCanvas* Canvas, const UBangoScriptComponent* ScriptComponent, FVector2f ScreenPos)
{
	if (!IsValid(ScriptComponent))
	{
		return;
	}
	
	FLinearColor TagColor = Bango::Colors::White;
	
	int32 U = 0;
	int32 V = 0;
	int32 UL = 64;
	int32 VL = 64;
		
	if (!ScriptComponent->GetScriptContainer().GetScriptClass().IsNull())
	{
		U = 64;
		V = 0;
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

	TagColor.A = 1.0f;
	
	float IconRawSize = 64.0f;
	float IconScale = 0.5f;
	float IconSize = IconRawSize * IconScale;
	
	{
		// Script Icon
		float X = ScreenPos.X - 0.5f * IconSize;
		float Y = ScreenPos.Y - 0.5f * IconSize;
		
		FCanvasIcon Icon = UCanvas::MakeIcon(Bango::Debug::GetScriptPIESprite(), U, V, UL, VL);
		Canvas->SetDrawColor(TagColor.ToFColor(false));
		Canvas->DrawIcon(Icon, X, Y, IconScale);
	}
}


TSharedPtr<SWidget> UBangoScriptsDebugDrawService::CreatePopupTitle(UBangoScriptComponent& ScriptComponent)
{ 
	return SNew(SBox)
	.HAlign(HAlign_Center)
	[
		SNew(STextBlock)
		.Text(GetLabelText(ScriptComponent))
//		.TextStyle(FAppStyle::Get(), "SmallText")
	];
}

TSharedPtr<SWidget> UBangoScriptsDebugDrawService::CreateEditButton(UBangoScriptComponent& ScriptComponent)
{
	TWeakObjectPtr<UBangoScriptComponent> WeakScriptComponent = &ScriptComponent;
	
	return SNew(SButton)
	.Text(LOCTEXT("EditScriptButton_Text", "Edit"))
	.TextStyle(FAppStyle::Get(), "SmallText")
	.HAlign(HAlign_Center)
	.OnClicked_Lambda( [WeakScriptComponent] ()
	{
		if (WeakScriptComponent.IsValid())
		{
			UBangoScriptBlueprint* Blueprint = WeakScriptComponent->GetScriptBlueprint(true);
			//if (Blueprint)
			//{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Blueprint);
			//}
		}
		
		return FReply::Handled();
	});
}

TSharedPtr<SWidget> UBangoScriptsDebugDrawService::CreateNewButton(UBangoScriptComponent& ScriptComponent)
{
	TWeakObjectPtr<UBangoScriptComponent> WeakScriptComponent = &ScriptComponent;
	TWeakObjectPtr<UBangoScriptsDebugDrawService> WeakThis = this;
	
	return SNew(SButton)
	.Text(LOCTEXT("NewScriptButton_Text", "New Script"))
	.TextStyle(FAppStyle::Get(), "SmallText")
	.OnClicked_Lambda( [WeakThis, WeakScriptComponent] ()
	{
		if (WeakScriptComponent.IsValid())
		{
			TArray<UPackage*> Packages;
			
			FBangoScriptContainer& ScriptContainer = WeakScriptComponent->GetScriptContainer();
			IBangoScriptHolderInterface* ScriptHolder = Cast<IBangoScriptHolderInterface>(WeakScriptComponent.Get());
			check(ScriptHolder);
			
			ScriptContainer.SetNewLevelScriptRequested();
			
			FString BlueprintName = WeakScriptComponent->GetFName().ToString();
	
			FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(*ScriptHolder, BlueprintName);
		}
		
		if (WeakThis.IsValid())
		{
			WeakThis->HoverInfo.Reset();
		}
		
		return FReply::Handled();
	});
}

TSharedPtr<SWidget> UBangoScriptsDebugDrawService::CreateRunButton(UBangoScriptComponent& ScriptComponent)
{
	TWeakObjectPtr<UBangoScriptComponent> WeakScriptComponent = &ScriptComponent;
	
	return SNew(SButton)
	.Text(INVTEXT("Run"))
	.TextStyle(FAppStyle::Get(), "SmallText")
	.OnClicked_Lambda([WeakScriptComponent]()
	{
		if (WeakScriptComponent.IsValid())
		{
			WeakScriptComponent->Run();
		}
		return FReply::Handled();
	});	
}


#undef LOCTEXT_NAMESPACE
