#pragma once

#include "BangoScripts/Uncooked/K2Nodes/Base/_K2NodeBangoBase.h"
#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder.h"

#include "K2Node_BangoFinishScript.generated.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

UCLASS(MinimalAPI, DisplayName = "Finish")
class UK2Node_BangoFinishScript : public UK2Node_BangoBase
{
	GENERATED_BODY()

public:
	UK2Node_BangoFinishScript();
	
public:
	void AllocateDefaultPins() override;

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	
	FText GetTooltipText() const override;
	
	FLinearColor GetNodeTitleColor() const override;
	
	void ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph) override;
};

using namespace BangoNodeBuilder;

MAKE_NODE_TYPE(BangoFinishScript, UK2Node_BangoFinishScript, NORMAL_CONSTRUCTION, Exec);

inline void BangoFinishScript::Construct()
{
	AllocateDefaultPins();
	Exec = _Node->GetExecPin();
}

#undef LOCTEXT_NAMESPACE