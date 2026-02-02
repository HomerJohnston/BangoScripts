#pragma once

#include "BangoScripts/Uncooked/K2Nodes/Base/_K2NodeBangoBase.h"
#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder.h"
#include "TickableEditorObject.h"

#include "K2Node_BangoThis.generated.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

/** 
 * 'This' is a getter for the object that ran this script. Normally this node is automatically configured
 * during creation of a Level Script, but you can also open the details panel to configure it. You could 
 * use this, for example, to destroy a trigger after the script runs.
 */
UCLASS(MinimalAPI, DisplayName = "This")
class UK2Node_BangoThis : public UK2Node_BangoBase, public FTickableEditorObject
{
public:

private:
	GENERATED_BODY()

public:
	UK2Node_BangoThis();

protected:
	/** If this is NOT set, this node will read the script's default settings ('This' Class Type) */
	UPROPERTY(EditAnywhere)
	UClass* ClassType;
	
public:
	UClass* GetClassType() const { return ClassType; }
	
public:
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	void AllocateDefaultPins() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	FText GetTooltipText() const override;
	
	FLinearColor GetNodeTitleColor() const override;
	
	FLinearColor GetNodeTitleTextColor() const override;

	void ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph) override;	
	
	bool ShouldDrawCompact() const override { return true; }
	
	bool IsNodePure() const override { return true; }
	
	void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	
	void PrepareForCopying() override;
	
	TStatId GetStatId() const override;
	
	bool IsTickable() const override { return true; }
	
	void Tick(float DeltaTime) override;
};

using namespace BangoNodeBuilder;

MAKE_NODE_TYPE(BangoThis, UK2Node_BangoThis, NORMAL_CONSTRUCTION, This);

inline void BangoThis::Construct()
{
	AllocateDefaultPins();
	
	This = FindPin("This");
}

#undef LOCTEXT_NAMESPACE