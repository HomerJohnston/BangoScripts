#include "BangoDebugDraw_ScriptComponent.h"

#include "EditorViewportClient.h"
#include "LevelEditor.h"
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
	
	float Alpha = Canvas.GetAlpha(BillboardScreenPos.Z);
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
		if (ScriptComponent->GetRunOnBeginPlay())
		{
			TagColor = Bango::Colors::Green;
		}
	}
	
	FText LabelText = FText::FromString(ScriptComponent->GetScriptContainer().GetDescription());
	
	if (LabelText.IsEmpty())
	{
		LabelText = FText::FromString(ScriptComponent->GetName());
	}
	
	FVector2D TextSize = FVector2D::ZeroVector;
	UFont* Font = GEngine->GetLargeFont();
	
	TagColor.A *= Alpha;
	
	float Padding = 4.0f;
	float Border = 2.0f;
	float IconPadding = 4.0f;
	
	if (!LabelText.IsEmpty())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TextSize = FontMeasureService->Measure(LabelText.ToString(), Font->GetLegacySlateFontInfo());
	}

	float TotalWidth = 0;
	float TotalHeight = 16;
	
	if (TextSize.X > KINDA_SMALL_NUMBER)
	{
		TotalWidth += IconPadding + TextSize.X;
	}
	
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
	
	if (!LabelText.IsEmpty())
	{
		// Text
		float X = BillboardScreenPos.X - 0.5f * TotalWidth + 0.5f * IconPadding;
		float Y = BillboardScreenPos.Y;
		
		FCanvasTextItem Text(FVector2D(X + IconPadding, Y), LabelText, Font, Alpha * Bango::Colors::White);
		Text.bCentreY = true;
		Canvas->DrawItem(Text);
	}
}

void UBangoDebugDraw_ScriptComponent::DebugDrawPIE(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)
{
	UE_LOG(LogBangoEditor, Display, TEXT("I Am"));
}
