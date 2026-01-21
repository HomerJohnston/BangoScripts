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
#include "Modules/ModuleManager.h"
#include "Slate/SceneViewport.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

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
    float WidgetCenterY = BillboardScreenPos.Y + 2.0f * TotalHeight;
    
	{
		// Background
		float X = WidgetCenterX - 0.5f * TotalWidth - Padding;
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
		float X = WidgetCenterX - 0.5f * TotalWidth;
		float Y = WidgetCenterY;
		
		FCanvasTextItem Text(FVector2D(X, Y), LabelText, Font, Alpha * Bango::Colors::White);
		Text.bCentreY = true;
		Canvas->DrawItem(Text);
	}
}

void UBangoDebugDraw_ScriptComponent::DebugDrawPIE(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)
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
	
	float Alpha = Canvas.GetAlpha(ScriptBillboard->GetComponentLocation(), true);
	if (Alpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	BillboardScreenPos.Z = 0.0f;
	
	DebugDrawPIEImpl(Canvas, ScriptComponent, Alpha, BillboardScreenPos);
}

void UBangoDebugDraw_ScriptComponent::DebugDrawPIEImpl(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent, float Alpha, const FVector& BillboardScreenPos)
{
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

	/*
	FText LabelText = FText::FromString(ScriptComponent->GetScriptContainer().GetDescription());
	
	if (LabelText.IsEmpty())
	{
		LabelText = FText::FromString(ScriptComponent->GetName());
	}
	*/
	
	FVector2D TextSize = FVector2D::ZeroVector;
	UFont* Font = GEngine->GetLargeFont();
	
	TagColor.A *= Alpha;
	
	float IconRawSize = 64.0f;
	float IconScale = 0.5f;
	
	float IconSize = IconRawSize * IconScale;
	float Padding = 4.0f;
	float Border = 2.0f;
	float IconPadding = -4.0f;
	
	/*
	if (!LabelText.IsEmpty())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TextSize = FontMeasureService->Measure(LabelText.ToString(), Font->GetLegacySlateFontInfo());
	}
	*/
	
	float TotalWidth = IconSize;
	float TotalHeight = FMath::Max(IconSize, IconSize);
	
	if (TextSize.X > KINDA_SMALL_NUMBER)
	{
		TotalWidth += IconPadding + TextSize.X;
	}
	
	/*
	{
		// Background
		float X = BillboardScreenPos.X - 0.5f * TotalWidth - Padding;
		float Y = BillboardScreenPos.Y - 0.5f * TotalHeight - Padding;
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
	*/
	
	{
		// Script Icon
		float X = BillboardScreenPos.X - 0.5f * TotalWidth;
		float Y = BillboardScreenPos.Y - 0.5f * IconSize;
		
		//FCanvasIcon Icon = UCanvas::MakeIcon(Bango::Debug::GetScriptDebugDrawIcon(), U, V, UL, VL);
		FCanvasIcon Icon = UCanvas::MakeIcon(Bango::Debug::GetScriptPIESprite(), U, V, UL, VL);
		Canvas->SetDrawColor(TagColor.ToFColor(false));
		Canvas->DrawIcon(Icon, X, Y, IconScale);
	}
	
	/*
	if (!LabelText.IsEmpty())
	{
		// Text
		float X = BillboardScreenPos.X - 0.5f * TotalWidth + 0.5f * IconSize + 0.5f * IconPadding;
		float Y = BillboardScreenPos.Y;
		
		FCanvasTextItem Text(FVector2D(X + 0.5f * IconSize + IconPadding, Y), LabelText, Font, Alpha * Bango::Colors::White);
		Text.bCentreY = true;
		Canvas->DrawItem(Text);
	}
	*/
}

#undef LOCTEXT_NAMESPACE