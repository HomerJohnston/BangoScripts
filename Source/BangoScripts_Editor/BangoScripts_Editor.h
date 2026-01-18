// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#pragma once

#include "BangoScripts_Editor/GPGEditorModuleBase.h"
#include "Private/ClassViewerFilter/BangoClassViewerFilter.h"
#include "Modules/ModuleManager.h"

class FSlateStyleSet;

struct BANGOSCRIPTS_EDITOR_API FBangoAssetCategoryPaths : EAssetCategoryPaths
{
	static FAssetCategoryPath Bango;
};

class FBangoScripts_EditorModule : public IModuleInterface, public FGPGEditorModuleBase
{
	// ============================================================================================
	// SETTINGS
	// ============================================================================================

	static EAssetTypeCategories::Type BangoAssetCategory;
	
	// ============================================================================================
	// STATE
	// ============================================================================================
protected:
	
	static TSharedPtr<FSlateStyleSet> StyleSet;
	
	static TSharedPtr<FBangoScriptsGlobalClassViewerFilter> BangoClassViewerFilter;
		
	// ============================================================================================
	// API
	// ============================================================================================
public:
	void StartupModule() override;

	void ShutdownModule() override;
	
protected:
	void LateRegisterClassFilter();
};

DECLARE_LOG_CATEGORY_EXTERN(BangoEditor, Log, All);