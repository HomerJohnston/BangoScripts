#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoGotoStart.h"

#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoGotoDestination.h"
#include "BangoScripts/Uncooked/K2Nodes/Base/_BangoMenuSubcategories.h"
#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

UK2Node_BangoGotoStart::UK2Node_BangoGotoStart()
{
	bShowNodeProperties = true;
	MenuSubcategory = BangoSubcategories::Scripting;
}

void UK2Node_BangoGotoStart::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_BangoGotoStart::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	auto* OutExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	
	if (!bShowOutExecPin)
	{
		OutExecPin->bHidden = true;
	}
}

FText UK2Node_BangoGotoStart::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return INVTEXT("Goto (start)");
}

void UK2Node_BangoGotoStart::ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	Super::ExpandNode(Compiler, SourceGraph);

	const UEdGraphSchema_K2* Schema = Compiler.GetSchema();
	bool bIsErrorFree = true;

	namespace NB = BangoNodeBuilder;
	NB::Builder Builder(Compiler, SourceGraph, this, Schema, &bIsErrorFree, FVector2f(5, 5));
	
	// -----------------
	// Make nodes
	
	auto Node_This =					Builder.WrapExistingNode<NB::BangoGotoSource>(this);
	auto Node_Sequence =				Builder.MakeNode<NB::ExecutionSequence>(2, 0);

	// -----------------
	// Post-setup

	FString UniqueID = *Compiler.GetGuid(this);
	
	Builder.FinishDeferredNodes();
	
	// -----------------
	// Make connections

	ConnectionsMade = 0;
	
	// First input
	Builder.CopyExternalConnection(Node_This.Exec, Node_Sequence.Exec);

	if (bShowOutExecPin)
	{
		if (Node_This.Then->HasAnyConnections())
		{
			Builder.CopyExternalConnection(Node_This.Then, Node_Sequence->GetThenPinGivenIndex(ConnectionsMade++));
		}	
	}
	
	for (TObjectPtr<class UEdGraphNode> Node : SourceGraph->Nodes)
	{
		if (UK2Node_BangoGotoDestination* DestinationNode = Cast<UK2Node_BangoGotoDestination>(Node))
		{
			if (DestinationNode->GetRerouteName() == this->GetRerouteName())
			{
				while (!Node_Sequence->GetThenPinGivenIndex(ConnectionsMade))
				{
					Node_Sequence->AddInputPin();
				}
				
				Builder.CreateConnection(Node_Sequence->GetThenPinGivenIndex(ConnectionsMade++), DestinationNode->GetExpandedExecPin());
			}
		} 
	}
	
	// Done!
	if (!bIsErrorFree)
	{
		Compiler.MessageLog.Error(*LOCTEXT("InternalConnectionError", "Internal connection error. @@").ToString(), this);
	}
	
	// Disconnect ThisNode from the graph
	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE