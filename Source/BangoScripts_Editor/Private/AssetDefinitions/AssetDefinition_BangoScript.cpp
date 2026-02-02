#include "AssetDefinition_BangoScript.h"

#include "BangoScripts_Editor.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BlueprintEditor/BangoScriptBlueprintEditor.h"
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
	return UBangoScript::StaticClass();
}

// ----------------------------------------------

FLinearColor UAssetDefinition_BangoScript::GetAssetColor() const
{
	return FLinearColor::Black;
}

// ----------------------------------------------

FText UAssetDefinition_BangoScript::GetAssetDescription(const FAssetData& AssetData) const
{
	return LOCTEXT("BangoScriptBlueprint_AssetDescription", "A re-usable script asset, to be used by the actor component UBangoScriptCompoent, or in other places.");
}

// ----------------------------------------------

FText UAssetDefinition_BangoScript::GetAssetDisplayName() const
{
	return LOCTEXT("BangoScriptBlueprint_AssetDisplayName", "Bango Script Blueprint");
}

// ================================================================================================

UFactory_BangoScript::UFactory_BangoScript()
{
	SupportedClass = UBangoScript::StaticClass();
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