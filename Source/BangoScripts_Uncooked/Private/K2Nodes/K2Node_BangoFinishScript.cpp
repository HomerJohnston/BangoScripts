#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFinishScript.h"

#include "GraphEditorSettings.h"
#include "BangoScripts/Uncooked/K2Nodes/Base/_BangoMenuSubcategories.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

namespace BangoNodeBuilder
{
	struct BangoFinishScript;
}

UK2Node_BangoFinishScript::UK2Node_BangoFinishScript()
{
	MenuSubcategory = BangoSubcategories::Scripting;
}

void UK2Node_BangoFinishScript::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
}

FText UK2Node_BangoFinishScript::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("K2Node_BangoFinishScript_NodeTitle", "Finish Script");
}

FText UK2Node_BangoFinishScript::GetTooltipText() const
{
	return LOCTEXT("K2Node_BangoFinishScript_NodeTooltipText", 
		"This node is not required for simple scripts which have a simple execution path. Bango will auto-destroy simple scripts.\n"
		"\n"
		"For scripts that subscribe to and await outside events, you must use this node to close out the running script instance.");
}

FLinearColor UK2Node_BangoFinishScript::GetNodeTitleColor() const
{
	return 0.5f * GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
}

void UK2Node_BangoFinishScript::ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	Super::ExpandNode(Compiler, SourceGraph);

	const UEdGraphSchema_K2* Schema = Compiler.GetSchema();
	bool bIsErrorFree = true;

	namespace NB = BangoNodeBuilder;
	NB::Builder Builder(Compiler, SourceGraph, this, Schema, &bIsErrorFree, FVector2f(5, 5));
	
	// -----------------
	// Make nodes
	
	auto Node_This =					Builder.WrapExistingNode<NB::BangoFinishScript>(this);
	auto Node_CallFunction =			Builder.MakeNode<NB::CallFunction>(0, 0);
	// -----------------
	// Post-setup

	FString UniqueID = *Compiler.GetGuid(this);
	
	Node_CallFunction->SetFromFunction(UBangoScript::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBangoScript, Finish)));
	
	Builder.FinishDeferredNodes();
	
	// -----------------
	// Make connections

	Builder.CopyExternalConnection(Node_This.Exec, Node_CallFunction.Exec);
	
	// Done!
	if (!bIsErrorFree)
	{
		Compiler.MessageLog.Error(*LOCTEXT("InternalConnectionError", "Internal connection error. @@").ToString(), this);
	}
	
	// Disconnect ThisNode from the graph
	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE