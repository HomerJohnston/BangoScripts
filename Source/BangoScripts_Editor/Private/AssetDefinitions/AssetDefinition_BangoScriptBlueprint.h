#pragma once

#include "AssetDefinitionDefault.h"
#include "Factories/Factory.h"

#include "AssetDefinition_BangoScriptBlueprint.generated.h"

/*
 * This asset definition is used for opening a script blueprint and for the actual creation of a script instance
 */
UCLASS()
class UAssetDefinition_BangoScriptBlueprint : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	TSoftClassPtr<UObject> GetAssetClass() const override;
	
	FLinearColor GetAssetColor() const override;
	
	FText GetAssetDisplayName() const override;
	
	EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
