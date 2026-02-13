#pragma once
#include "K2Node.h"

#include "_K2NodeBangoBase.generated.h"

class UBangoScriptBlueprint;

#define LOCTEXT_NAMESPACE "BangoScripts"

UCLASS(Abstract, MinimalAPI)
class UK2Node_BangoBase : public UK2Node
{
	GENERATED_BODY()

public:
	UK2Node_BangoBase();
	
protected:
	bool bIsLatent = false;
	bool bHideLatentIcon = false;
	bool bShowNodeProperties = false;
	
	// Override this in child nodes to change the subcategory (all custom K2 Nodes will be in Bango|MenuSubcategory)
	FText MenuSubcategory;
	
public:
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	
	FText GetMenuCategory() const override;
	
	bool IsLatentForMacros() const override;

	FName GetCornerIcon() const override;
	
	FText GetToolTipHeading() const override;

	bool ShouldShowNodeProperties() const override;
	
	FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	
	FLinearColor GetNodeTitleColor() const override;
	
	FLinearColor GetNodeTitleTextColor() const override;
	
	virtual void FixUpForNewOwnerActor(const TSoftObjectPtr<AActor>& OldOwner, const TSoftObjectPtr<AActor>& NewOwner) { };
	
	BANGOSCRIPTS_UNCOOKED_API UBangoScriptBlueprint* GetBangoScriptBlueprint() const;
};

#undef LOCTEXT_NAMESPACE
