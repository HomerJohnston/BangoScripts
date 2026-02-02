#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoGotoDestination.h"

#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoGotoStart.h"
#include "BangoScripts/Uncooked/K2Nodes/Base/_BangoMenuSubcategories.h"
#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder_Macros.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

UK2Node_BangoGotoDestination::UK2Node_BangoGotoDestination()
{
	bShowNodeProperties = true;
	MenuSubcategory = BangoSubcategories::Scripting;
}

void UK2Node_BangoGotoDestination::AllocateDefaultPins()
{
	UEdGraphPin* InputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	InputPin->bHidden = true;
	
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

FText UK2Node_BangoGotoDestination::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return INVTEXT("Goto (end)");
}

void UK2Node_BangoGotoDestination::ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	Super::ExpandNode(Compiler, SourceGraph);

	const UEdGraphSchema_K2* Schema = Compiler.GetSchema();
	bool bIsErrorFree = true;

	namespace NB = BangoNodeBuilder;
	NB::Builder Builder(Compiler, SourceGraph, this, Schema, &bIsErrorFree, FVector2f(5, 5));
	
	// -----------------
	// Make nodes
	
	//using namespace Bango_NodeBuilder;
	auto Node_This =					Builder.WrapExistingNode<NB::BangoGotoDestination>(this);
	auto Node_Sequence =				Builder.MakeNode<NB::ExecutionSequence>(1, 0);
	
	// -----------------
	// Post-setup

	FString UniqueID = *Compiler.GetGuid(this);
	
	Builder.FinishDeferredNodes();
	
	// -----------------
	// Make connections

	bool bFoundConnection = false;
	
	if (!Node_This.Exec->HasAnyConnections())
	{
		// We haven't been wired up yet. Let's see if we can do it ourselves?
		
		for (TObjectPtr<class UEdGraphNode> Node : SourceGraph->Nodes)
		{
			if (UK2Node_BangoGotoStart* SourceNode = Cast<UK2Node_BangoGotoStart>(Node))
			{
				if (SourceNode->GetRerouteName() == this->GetRerouteName())
				{
					if (bFoundConnection)
					{
						bIsErrorFree = false;
						Compiler.MessageLog.Error(TEXT("Check your GOTO nodes; there is more than one source node sharing the same name."));
						break;
					}
					
					bFoundConnection = true;
				}
			} 
		}
	}
	
	// We replace ourselves with a Sequence node because it's easy and I am not going to be alive forever
	Builder.MoveExternalConnection(Node_This.Exec, Node_Sequence.Exec);
	Builder.MoveExternalConnection(Node_This.Then, Node_Sequence->GetThenPinGivenIndex(0));
	
	// This will help tell GotoStart nodes to hook onto the sequence exec pin instead of our node pin
	ExpandedExecPin = Node_Sequence.Exec; //Node_Knot.InputPin;
	
	// Done!
	if (!bIsErrorFree)
	{
		Compiler.MessageLog.Error(*LOCTEXT("InternalConnectionError", "Internal connection error. @@").ToString(), this);
	}
	
	// Disconnect ThisNode from the graph
	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_BangoGotoDestination::GetExpandedExecPin()
{
	if (ExpandedExecPin)
	{
		return ExpandedExecPin;
	}
	
	return GetExecPin();
}

class FNodeHandlingFunctor* UK2Node_BangoGotoDestination::CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

#undef LOCTEXT_NAMESPACE
