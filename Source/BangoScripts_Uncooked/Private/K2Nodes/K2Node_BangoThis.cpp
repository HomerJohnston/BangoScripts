#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoThis.h"

#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Uncooked/K2Nodes/Base/_BangoMenuSubcategories.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

UK2Node_BangoThis::UK2Node_BangoThis()
{
	bShowNodeProperties = true;	
	MenuSubcategory = BangoSubcategories::Scripting;
}

void UK2Node_BangoThis::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	ReconstructNode();
}

void UK2Node_BangoThis::AllocateDefaultPins()
{
	auto* ThisPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, FName("This"));
	
	if (ClassType)
	{
		ThisPin->PinType.PinSubCategoryObject = ClassType;
	}
	else
	{
		UBlueprint* BP = GetBlueprint();
		
		if (BP)
		{
			UClass* GeneratedClass = BP->GeneratedClass;
			
			if (GeneratedClass)
			{
				UObject* GeneratedClassInstance = GeneratedClass->GetDefaultObject();
				UBangoScript* ScriptInstance = Cast<UBangoScript>(GeneratedClassInstance);
		
				if (ScriptInstance)
				{
					TSubclassOf<UObject> BPClassType = ScriptInstance->GetThis_ClassType();
		
					if (BPClassType)
					{
						ThisPin->PinType.PinSubCategoryObject = BPClassType;
					}		
				}	
			}
		}
	}
}

FText UK2Node_BangoThis::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("ThisNode_Title", "This");
}

FText UK2Node_BangoThis::GetTooltipText() const
{
	return Super::GetTooltipText();
}

FLinearColor UK2Node_BangoThis::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

FLinearColor UK2Node_BangoThis::GetNodeTitleTextColor() const
{
	return Super::GetNodeTitleTextColor();
}

void UK2Node_BangoThis::ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	Super::ExpandNode(Compiler, SourceGraph);

	const UEdGraphSchema_K2* Schema = Compiler.GetSchema();
	bool bIsErrorFree = true;

	namespace NB = BangoNodeBuilder;
	NB::Builder Builder(Compiler, SourceGraph, this, Schema, &bIsErrorFree, FVector2f(5, 5));
	
	// -----------------
	// Make nodes
	
	auto Node_This =					Builder.WrapExistingNode<NB::BangoThis>(this);
	auto Node_ThisLiteral =				Builder.MakeNode<NB::VariableGet>(1,1);
	auto Node_CastToType =				Builder.MakeNode<NB::DynamicCast_Pure>(1, 1);
	
	// -----------------
	// Post-setup

	FString UniqueID = *Compiler.GetGuid(this);
	
	Node_ThisLiteral->VariableReference.SetSelfMember("This");
	Node_CastToType->TargetType = Cast<UClass>(Node_This.This->PinType.PinSubCategoryObject);
	
	Builder.FinishDeferredNodes();
	
	// -----------------
	// Make connections

	// First input
	Builder.CreateConnection(Node_ThisLiteral.Output, Node_CastToType.ObjectToCast);
	
	// Final output
	Builder.MoveExternalConnection(Node_This.This, Node_CastToType.CastedObject);
	
	// Done!
	if (!bIsErrorFree)
	{
		Compiler.MessageLog.Error(*LOCTEXT("InternalConnectionError", "Internal connection error. @@").ToString(), this);
	}
	
	// Disconnect ThisNode from the graph
	BreakAllNodeLinks();
}

void UK2Node_BangoThis::PreDuplicate(FObjectDuplicationParameters& DupParams)
{
	Super::PreDuplicate(DupParams);
}

void UK2Node_BangoThis::PrepareForCopying()
{
	Super::PrepareForCopying();
}

TStatId UK2Node_BangoThis::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FBangoEditor, STATGROUP_Tickables);
}

void UK2Node_BangoThis::Tick(float DeltaTime)
{
	if (ClassType || !HasValidBlueprint())
	{
		return;
	}
	
	auto ThisPin = GetPinAt(0);
	
	UBlueprint* BP = GetBlueprint();
	
	if (BP)
	{
		UClass* GeneratedClass = BP->GeneratedClass;
		
		if (GeneratedClass)
		{
			UObject* GeneratedClassInstance = GeneratedClass->GetDefaultObject();
			UBangoScript* ScriptInstance = Cast<UBangoScript>(GeneratedClassInstance);
	
			if (ScriptInstance)
			{
				TSubclassOf<UObject> BPClassType = ScriptInstance->GetThis_ClassType();
	
				if (BPClassType)
				{
					if (Cast<UClass>(ThisPin->PinType.PinSubCategoryObject.Get()) != BPClassType)
					{
						ReconstructNode();
					}
				}		
			}	
		}
	}
}

#undef LOCTEXT_NAMESPACE
