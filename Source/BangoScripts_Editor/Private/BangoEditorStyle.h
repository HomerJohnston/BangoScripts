// Copyright Ghost Pepper Games, Inc. All Rights Reserved.
// This work is MIT-licensed. Feel free to use it however you wish, within the confines of the MIT license.

#pragma once

#include "Styling/SlateStyle.h"
#include "Delegates/IDelegateInstance.h"
#include "Textures/SlateIcon.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ==============================================

struct FBangoBrushes
{
    FName None;

    FName Icon_Hourglass;
	FName Icon_PauseHand;
	FName Icon_Plunger;
	FName Icon_Plunger_Dim;
	FName Icon_PortalIn;
	FName Icon_PortalOut;
	
	FName Icon_EditScript;
		
	FName Border_InlineBlueprintGraph;
};

extern FBangoBrushes BangoEditorBrushes;

class FBangoEditorStyle
{
public:
    
public:
    FBangoEditorStyle();
	
    virtual ~FBangoEditorStyle();
	
public:
    static ISlateStyle& Get();

    static const FSlateBrush* GetImageBrush(FName BrushName);

    static FName GetStyleSetName();

    static void Initialize();

    static void ReloadTextures();
	
protected:
    static TSharedRef< class FSlateStyleSet > Create();
	
    static void SetupStyles();
	
    static void OnPatchComplete();

    static const ISlateStyle* GetParentStyle();
	
    static TArray<TStrongObjectPtr<UTexture2D>> Textures;

    static FDelegateHandle OnPatchCompleteHandle;

    static TSharedPtr<FSlateStyleSet> StyleInstance;

};

#undef LOCTEXT_NAMESPACE