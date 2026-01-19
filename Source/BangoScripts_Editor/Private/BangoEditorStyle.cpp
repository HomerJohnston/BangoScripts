// Unlicensed. This file is public domain.

#include "BangoEditorStyle.h"

#include "BangoScripts/Components/BangoActorIDComponent.h"
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

TArray<TStrongObjectPtr<UTexture2D>> FBangoEditorStyle::Textures;
FDelegateHandle FBangoEditorStyle::OnPatchCompleteHandle;
TSharedPtr<FSlateStyleSet> FBangoEditorStyle::StyleInstance = nullptr;

//FBangoFonts BangoEditorFonts;
FBangoBrushes BangoEditorBrushes;
//FBangoStyles BangoEditorStyles;

// ================================================================================================

#define BANGO_QUOTE(X) #X

// ------------------------------------------------------------------------------------------------

/** Makes a simple font definition copying default font */
#define BANGO_DEFINE_FONT(NAME, STYLE, SIZE)\
	BangoEditorFonts.NAME = DEFAULT_FONT(STYLE, SIZE);\
	FSlateFontInfo& NAME = BangoEditorFonts.NAME

// ------------------------------------------------------------------------------------------------

/** Loads a TTF from disk */
#define BANGO_LOAD_FONT(NAME, RESOURCE_PATH, SIZE)\
	TSharedRef<FCompositeFont> SourceCompositeFont_##NAME = MakeShared<FStandaloneCompositeFont>();\
	SourceCompositeFont_##NAME->DefaultTypeface.AppendFont(TEXT("Regular"), BangoEditor::FileUtilities::GetResourcesFolder() / RESOURCE_PATH, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);\
	BangoEditorFonts.NAME = FSlateFontInfo(SourceCompositeFont_##NAME, SIZE);\
	FSlateFontInfo& NAME = BangoEditorFonts.NAME

// ------------------------------------------------------------------------------------------------

/** Define a new brush */
#define BANGO_DEFINE_BRUSH(TYPE, BRUSHNAME, FILENAME, EXTENSION, ...)\
	BangoEditorBrushes.BRUSHNAME = BANGO_QUOTE(BRUSHNAME);\
	StyleInstance->Set(BANGO_QUOTE(BRUSHNAME), new TYPE(StyleInstance->RootToContentDir(FILENAME, TEXT(EXTENSION)), __VA_ARGS__));\
	const TYPE& BRUSHNAME = *static_cast<const TYPE*>(StyleInstance->GetBrush(BANGO_QUOTE(BRUSHNAME)))

// ------------------------------------------------------------------------------------------------

/** Define a new style */
#define BANGO_DEFINE_STYLE(TYPE, STYLENAME, TEMPLATE, MODS)\
	BangoEditorStyles.STYLENAME = BANGO_QUOTE(STYLENAME);\
	StyleInstance->Set(BANGO_QUOTE(STYLENAME), TYPE(TEMPLATE));\
	TYPE& STYLENAME = const_cast<TYPE&>(StyleInstance->GetWidgetStyle<TYPE>(BANGO_QUOTE(STYLENAME)));\
	STYLENAME MODS
	
// ------------------------------------------------------------------------------------------------

/** Used to copy an existing UE brush into BangoEditor style for easier use */
#define BANGO_REDEFINE_UE_BRUSH(TYPE, BANGONAME, UESTYLESET, UENAME, ...)\
	BangoEditorBrushes.BANGONAME = BANGO_QUOTE(BANGONAME);\
	const TYPE& BANGONAME = *(new TYPE(UESTYLESET::GetBrush(UENAME)->GetResourceName().ToString(), __VA_ARGS__));\
	StyleInstance->Set(BANGO_QUOTE(BANGONAME), const_cast<TYPE*>(&BANGONAME))

// ================================================================================================

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleInstance->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define BOX_BRUSH_SVG( RelativePath, ... ) FSlateVectorBoxBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define BORDER_BRUSH_SVG( RelativePath, ... ) FSlateVectorBorderBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

#define CORE_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleInstance->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CORE_BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleInstance->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define CORE_BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleInstance->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

#define CORE_IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(StyleInstance->RootToCoreContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define CORE_BOX_BRUSH_SVG( RelativePath, ... ) FSlateVectorBoxBrush(StyleInstance->RootToCoreContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define CORE_BORDER_BRUSH_SVG( RelativePath, ... ) FSlateVectorBorderBrush(StyleInstance->RootToCoreContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

// ================================================================================================


ISlateStyle& FBangoEditorStyle::Get()
{
	TSharedPtr<FSlateStyleSet> FUFKYOU = StyleInstance;
	return *StyleInstance;
}

const FSlateBrush* FBangoEditorStyle::GetImageBrush(FName BrushName)
{
	return Get().GetBrush(BrushName);
}

FBangoEditorStyle::FBangoEditorStyle()
{
	
}

FBangoEditorStyle::~FBangoEditorStyle()
{
	Textures.Empty();
	
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
}

FName FBangoEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("BangoEditorEditorStyle"));
	return StyleSetName;
}

void FBangoEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}

	SetupStyles();

	ReloadTextures();
}

void FBangoEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

TSharedRef<class FSlateStyleSet> FBangoEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetParentStyleName(FAppStyle::GetAppStyleSetName());
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("BangoScripts")->GetBaseDir() / TEXT("Resources"));
	Style->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	
	return Style;
}

const ISlateStyle* FBangoEditorStyle::GetParentStyle()
{
	return &FAppStyle::Get();
}

#define BANGO_COMMON_BRUSH "Common/ButtonHoverHint"
#define BANGO_COMMON_MARGIN FMargin(4.0 / 16.0)
#define BANGO_COMMON_PRESSED_PADDING FMargin(0, 1, 0, -1) // Push down by one pixel
#define BANGO_COMMON_CHECKBOXSTYLE FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox")


void FBangoEditorStyle::SetupStyles()
{
	if (!IsRunningCommandlet())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
	
	
	BANGO_REDEFINE_UE_BRUSH(FSlateImageBrush,			None,							FAppStyle,	"NoBorder",					FVector2f(16, 16), Bango::Colors::Transparent);

	// ============================================================================================
	// FONTS
	// Format: BANGO_DEFINE_FONT(FName DefinitionName, FString StyleName, int Size);
	// Sample: BANGO_DEFINE_FONT(Font_Basic, "Normal", 10);
	// ============================================================================================


	// ============================================================================================
	// BRUSHES
	// Format: BANGO_DEFINE_BRUSH(TypeName, FNameIcon_AudioTime, "DialogueNodeIcons/AudioTime", ".png", FVector2f(16, 16));
	// Sample: BANGO_DEFINE_BRUSH(FSlateImageBrush, Icon_AudioTime, "DialogueNodeIcons/AudioTime", ".png", FVector2f(16, 16));
	// ============================================================================================
	// BANGO_DEFINE_BRUSH(FSlateImageBrush,			Icon_AudioTime,					"DialogueNodeIcons/AudioTime", ".png",	FVector2f(16, 16));

	// ============================================================================================
	// BRUSHES - SVGs
	// ============================================================================================
	//	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_Chevron_Right,				"Icon_Chevron_Right", ".svg",			FVector2f(16, 16), Bango::Colors::White);
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_Hourglass,						"NodeIcons/Icon_Hourglass", ".svg",				FVector2f(20, 20), Bango::Colors::White);
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_PauseHand,						"NodeIcons/Icon_PauseHand", ".svg",				FVector2f(20, 20), Bango::Colors::White);
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_Plunger,						"NodeIcons/Icon_Plunger", ".svg",				FVector2f(20, 20), Bango::Colors::White);
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_Plunger_Dim,					"NodeIcons/Icon_Plunger", ".svg",				FVector2f(20, 20), Bango::Colors::White_Trans);
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_PortalIn,						"NodeIcons/Icon_PortalIn", ".svg",				FVector2f(20, 20), Bango::Colors::White);
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_PortalOut,						"NodeIcons/Icon_PortalOut", ".svg",				FVector2f(20, 20), Bango::Colors::White);
	
	BANGO_DEFINE_BRUSH(FSlateVectorImageBrush,	Icon_EditScript,					"ButtonIcons/Icon_EditScript", ".svg",			FVector2f(20, 20), Bango::Colors::White);
	
	BANGO_DEFINE_BRUSH(FSlateBorderBrush,		Border_InlineBlueprintGraph,		"Textures/Border_2px_White_Sharp", ".png",		FMargin(4.0f / 8.0f));
	
	// ============================================================================================
	// SLIDER STYLES
	// ============================================================================================

	/*
	BANGO_DEFINE_STYLE(FSliderStyle, SliderStyle_FragmentTimePadding, FSliderStyle::GetDefault(),
		.SetBarThickness(0.f)
		.SetNormalThumbImage(IMAGE_BRUSH("ProgressBar_Fill", CoreStyleConstants::Icon8x8, BangoEditorColor::Gray))
		.SetHoveredThumbImage(IMAGE_BRUSH("ProgressBar_Fill", CoreStyleConstants::Icon8x8, BangoEditorColor::LightGray))
	);
	*/
	
	// ============================================================================================
	// BUTTON STYLES
	// ============================================================================================

	// ============================================================================================
	// COMBO BUTTON STYLES
	// ============================================================================================

	/*
	BANGO_DEFINE_STYLE(FComboButtonStyle, ComboButtonStyle_BangoEditorGameplayTagTypedPicker, FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton"),
		.SetButtonStyle(ButtonStyle_TagButton)
		.SetDownArrowPadding(FMargin(0, 2, 0, 0))
		.SetDownArrowAlignment(VAlign_Top)
		);
	*/
	
	// ============================================================================================
	// CHECKBOX STYLES
	// ============================================================================================

	/*
	BANGO_DEFINE_STYLE(FCheckBoxStyle, CheckBoxStyle_TypeSettingsOverride, BANGO_COMMON_CHECKBOXSTYLE,
		.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
		.SetForegroundColor(BangoEditorColor::Gray_Trans) // Unchecked
		.SetHoveredForegroundColor(BangoEditorColor::White) // Unchecked, Hovered
		.SetPressedForegroundColor(BangoEditorColor::LightGray) // Unchecked, Pressed
		.SetCheckedForegroundColor(BangoEditorColor::LightGray) // Checked
		.SetCheckedHoveredForegroundColor(BangoEditorColor::White) // Checked, Hovered
		.SetCheckedPressedForegroundColor(BangoEditorColor::LightGray) // Checked, Pressed
		
		.SetCheckedImage(Icon_Switch_On)
		.SetCheckedHoveredImage(Icon_Switch_On)
		.SetCheckedPressedImage(Icon_Switch_On)

		.SetUncheckedImage(Icon_Switch_Off)
		.SetUncheckedHoveredImage(Icon_Switch_Off)
		.SetUncheckedPressedImage(Icon_Switch_Off)
	);
	*/
	
	// ============================================================================================
	// SCROLLBAR STYLES
	// ============================================================================================

	/*
	BANGO_DEFINE_STYLE(FScrollBarStyle, ScrollBarStyle_DialogueBox, FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"),
		.SetThickness(2)
		.SetHorizontalBackgroundImage(Box_SolidBlack)
		.SetHorizontalBottomSlotImage(Box_SolidWhite)
		.SetDraggedThumbImage(Box_SolidWhite)
		.SetHoveredThumbImage(Box_SolidWhite)
		.SetNormalThumbImage(Box_SolidLightGray)
	);
	*/
	
	// ============================================================================================
	// TEXT BLOCK STYLES
	// ============================================================================================

	/*
	BANGO_DEFINE_STYLE(FTextBlockStyle, TextBlockStyle_DialogueText, GetParentStyle()->GetWidgetStyle<FTextBlockStyle>("NormalText"),
		.SetFont(Font_DialogueText)
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetFontSize(10)
	);
	*/

	int32 DefaultSize = FCoreStyle::RegularTextSize + 2;
	
	
	//StyleInstance->Set("RichTextBlock.TextHighlight", FTextBlockStyle(TextStyle_Normal));
	
	//StyleInstance->Set("RichTextBlock.Bold", FTextBlockStyle(TextStyle_Normal));

	//StyleInstance->Set("RichTextBlock.BoldHighlight", FTextBlockStyle(TextStyle_Normal));

	{
		const FString BangoScriptComponentIconPropName = FString::Printf(TEXT("ClassIcon.%s"), *UBangoScriptComponent::StaticClass()->GetName());
		StyleInstance->Set(*BangoScriptComponentIconPropName, new FSlateVectorImageBrush(StyleInstance->RootToContentDir("NodeIcons/Icon_Script", L".svg"), FVector2f(20, 20), Bango::Colors::Yellow));
	}
	{
		const FString BangoActorIDComponentIconPropName = FString::Printf(TEXT("ClassIcon.%s"), *UBangoActorIDComponent::StaticClass()->GetName());
		StyleInstance->Set(*BangoActorIDComponentIconPropName, new FSlateVectorImageBrush(StyleInstance->RootToContentDir("NodeIcons/Icon_ActorID", L".svg"), FVector2f(20, 20), Bango::Colors::Yellow));
	}
	
	// ============================================================================================
	// EDITABLE TEXT BLOCK STYLES
	// ============================================================================================
	
	// ============================================================================================
	// PROGRESS BAR STYLES
	// ============================================================================================

	// ============================================================================================
	// HYPERLINK STYLES
	// ============================================================================================

	// ============================================================================================
	// OTHER
	// ============================================================================================
}

#undef LOCTEXT_NAMESPACE