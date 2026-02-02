#pragma once
#include "AssetDefinitionDefault.h"
#include "Factories/Factory.h"

#include "AssetDefinition_BangoScript.generated.h"

UCLASS()
class UAssetDefinition_BangoScript : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	
	TSoftClassPtr<UObject> GetAssetClass() const override;
	
	FLinearColor GetAssetColor() const override;
	
	FText GetAssetDescription(const FAssetData& AssetData) const override;
	
	FText GetAssetDisplayName() const override;
	
	EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};

UCLASS()
class UFactory_BangoScript : public UFactory
{
	GENERATED_BODY()
	
	UFactory_BangoScript();

	UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};