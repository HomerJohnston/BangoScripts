#include "AssetDefinition_BangoScript.h"

#include "BangoScripts_Editor.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BlueprintEditor/BangoBlueprintEditor.h"
#include "Kismet2/KismetEditorUtilities.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ----------------------------------------------

TConstArrayView<FAssetCategoryPath> UAssetDefinition_BangoScript::GetAssetCategories() const
{
	static const auto Categories = { FBangoAssetCategoryPaths::Bango };

	return Categories;
}

// ----------------------------------------------

TSoftClassPtr<UObject> UAssetDefinition_BangoScript::GetAssetClass() const
{
	return UBangoScriptBlueprint::StaticClass();
}

FLinearColor UAssetDefinition_BangoScript::GetAssetColor() const
{
	// TODO
	return FLinearColor::Black;
}

// ----------------------------------------------

FText UAssetDefinition_BangoScript::GetAssetDescription(const FAssetData& AssetData) const
{
	return LOCTEXT("BangoScriptBlueprint_AssetDescription", "A re-usable script asset, to be used by the actor component UBangoScriptCompoent, or in other places.");
}

FText UAssetDefinition_BangoScript::GetAssetDisplayName() const
{
	return LOCTEXT("BangoScriptBlueprint_AssetDisplayName", "Bango Script Blueprint");
}

// ----------------------------------------------

EAssetCommandResult UAssetDefinition_BangoScript::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	if (Subsystem)
	{
		TArray<UBlueprint*> ScriptBlueprints = OpenArgs.LoadObjects<UBlueprint>();
		
		if (ScriptBlueprints.Num() == 1)
		{
			TSharedRef<FBangoBlueprintEditor> NewBlueprintEditor(new FBangoBlueprintEditor());

			const bool bShouldOpenInDefaultsMode = false;

			NewBlueprintEditor->InitBlueprintEditor(EToolkitMode::Standalone, nullptr, ScriptBlueprints, bShouldOpenInDefaultsMode);

			return EAssetCommandResult::Handled;
		}
	}

	return EAssetCommandResult::Unhandled;
}

// ----------------------------------------------

UFactory_BangoScript::UFactory_BangoScript()
{
	SupportedClass = UBangoScriptBlueprint::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

// ----------------------------------------------

UObject* UFactory_BangoScript::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FKismetEditorUtilities::CreateBlueprint(InClass, InParent, InName, BPTYPE_Normal, UBangoScriptBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
}

// ----------------------------------------------

#undef LOCTEXT_NAMESPACE