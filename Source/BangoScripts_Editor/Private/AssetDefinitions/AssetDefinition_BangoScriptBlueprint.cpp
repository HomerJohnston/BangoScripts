#include "AssetDefinition_BangoScriptBlueprint.h"

#include "BangoScripts_Editor.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BlueprintEditor/BangoScriptBlueprintEditor.h"
#include "Kismet2/KismetEditorUtilities.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ----------------------------------------------

TSoftClassPtr<UObject> UAssetDefinition_BangoScriptBlueprint::GetAssetClass() const
{
	return UBangoScriptBlueprint::StaticClass();
}

// ----------------------------------------------

FLinearColor UAssetDefinition_BangoScriptBlueprint::GetAssetColor() const
{
	return FLinearColor::Gray;
}

FText UAssetDefinition_BangoScriptBlueprint::GetAssetDisplayName() const
{
	return LOCTEXT("BangoScriptBlueprint_AssetDisplayName", "Bango Script Blueprint");
}

// ----------------------------------------------

EAssetCommandResult UAssetDefinition_BangoScriptBlueprint::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	if (Subsystem)
	{
		TArray<UBlueprint*> ScriptBlueprints = OpenArgs.LoadObjects<UBlueprint>();
		
		if (ScriptBlueprints.Num() == 1)
		{
			TSharedRef<FBangoScriptBlueprintEditor> NewBlueprintEditor(new FBangoScriptBlueprintEditor());

			const bool bShouldOpenInDefaultsMode = false;

			NewBlueprintEditor->InitBlueprintEditor(EToolkitMode::Standalone, nullptr, ScriptBlueprints, bShouldOpenInDefaultsMode);

			return EAssetCommandResult::Handled;
		}
	}

	return EAssetCommandResult::Unhandled;
}
