#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFindActor.h"

#include "K2Node_DynamicCast.h"
#include "Selection.h"
#include "UnrealEdGlobals.h"
#include "BangoScripts/Subsystem/BangoActorIDSubsystem.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder.h"
#include "Editor/UnrealEdEngine.h"
#include "WorldPartition/ActorDescContainerInstance.h"
#include "WorldPartition/WorldPartition.h"

#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder_Macros.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

UK2Node_BangoFindActor::UK2Node_BangoFindActor()
{
	bShowNodeProperties = true;
}

bool UK2Node_BangoFindActor::ShouldDrawCompact() const
{
	return !TargetActor.IsNull();
}

FLinearColor UK2Node_BangoFindActor::GetNodeBodyTintColor() const
{
	return Bango::Colors::LightBlue;
}

FLinearColor UK2Node_BangoFindActor::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

FLinearColor UK2Node_BangoFindActor::GetNodeTitleTextColor() const
{
	return Bango::Colors::Orange;
}

FText UK2Node_BangoFindActor::GetTooltipText() const
{
	if (!TargetActor.IsNull())
	{
		return LOCTEXT("TooltipText_BangoFindActorNode_TargetActor", "Soft actor pointer lookup");
	}
	
	return LOCTEXT("TooltipText_BangoFindActorNode_BangoName", "Bango ID Component lookup");
}

void UK2Node_BangoFindActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.MemberProperty == StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(ThisClass, CastTo)))
	{
		ReconstructNode();
	}
}

void UK2Node_BangoFindActor::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	
	Color = FLinearColor::MakeRandomColor();
}

void UK2Node_BangoFindActor::AllocateDefaultPins()
{
	auto* SoftActorPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_SoftObject, FName("SoftActor"));
	auto* BangoNamePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, FName("BangoName"));
	auto* BangoGuidPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FName("BangoGuid")); 
	
	auto* FoundActorPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, FName("FoundActor"));
	
	// Always hidden
	SoftActorPin->bHidden = true;
	
	// This pin is hidden when the node is set to a specific target actor
	BangoNamePin->bHidden = !TargetActor.IsNull(); 
	
	// Make it show an empty label // TODO is there a nicer way to do this?
	BangoNamePin->PinFriendlyName = LOCTEXT("BangoFindActorNode_BangoNamePinLabel", " ");
	
	// Never need to edit this pin, it's always null or automatically set
	BangoGuidPin->bHidden = true;
	
	if (IsValid(CastTo))
	{
		FoundActorPin->PinType.PinSubCategoryObject = CastTo;
	}
	else
	{
		FoundActorPin->PinType.PinSubCategoryObject = AActor::StaticClass();		
	}
	
	FoundActorPin->PinFriendlyName = LOCTEXT("BangoFindActorNode_FoundActorLabel", " ");
}

FText UK2Node_BangoFindActor::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!TargetActor.IsNull())
	{
		if (TargetActor.IsValid())
		{
			return FText::FromString(TargetActor->GetActorLabel());
		}
		
		UWorld* World = GetWorld();
	
		if (World)
		{
			UWorldPartition* WorldPartition = World->GetWorldPartition();
		
			if (WorldPartition)
			{
				UActorDescContainerInstance* ActorDescContainer = WorldPartition->GetActorDescContainerInstance();
			
				if (ActorDescContainer)
				{
					const FWorldPartitionActorDescInstance* ActorDesc = ActorDescContainer->GetActorDescInstanceByPath(GetTargetActor().ToSoftObjectPath());
				
					if (ActorDesc)
					{
						return FText::FromName(ActorDesc->GetActorLabel());
					}
				}
			}
		}
		
		// Fall back to pulling a name out of the raw path
		FString SubPath = TargetActor.ToSoftObjectPath().GetSubPathString();
		
		int32 LastDotIndex;
		if (SubPath.FindLastChar(TEXT('.'), LastDotIndex))
		{
			FString ActorName = SubPath.Mid(LastDotIndex + 1);
			
			return FText::Format(LOCTEXT("BangoFindActorNodeTitle_Unloaded", "{0}"), FText::FromString(ActorName));
		}
		
		checkNoEntry();
	}
	
	return LOCTEXT("BangoFindActorNode_Title", "Find Actor");
}

void UK2Node_BangoFindActor::ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	if (!TargetActor.IsNull())
	{
		ExpandNode_SoftActor(Compiler, SourceGraph);
	}
	else
	{
		ExpandNode_ManualName(Compiler, SourceGraph);
	}
}

void UK2Node_BangoFindActor::ExpandNode_SoftActor(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	Super::ExpandNode(Compiler, SourceGraph);

	const UEdGraphSchema_K2* Schema = Compiler.GetSchema();
	bool bIsErrorFree = true;

	namespace NB = BangoNodeBuilder;
	NB::Builder Builder(Compiler, SourceGraph, this, Schema, &bIsErrorFree, FVector2f(5, 5));
	
	// -----------------
	// Make nodes
	
	auto Node_This =					Builder.WrapExistingNode<NB::BangoFindActor>(this);
	auto Node_ResolveSoft =				Builder.MakeNode<NB::ConvertAsset>(1, 1);
	auto Node_CastToType =				Builder.MakeNode<NB::DynamicCast_Pure>(1, 1);
	auto Node_SoftObjectPath =			Builder.MakeNode<NB::CallFunction>(1, 1);
	auto Node_SoftObjectRef =			Builder.MakeNode<NB::CallFunction>(1, 1);
	auto Node_ResolveObject =			Builder.MakeNode<NB::CallFunction>(1, 1);
	
	/*
	*
	UK2Node_CallFunction* ConvertToSoftObjectRef = Compiler->SpawnIntermediateNode<UK2Node_CallFunction>(OriginNode, Graph);
	ConvertToSoftObjectRef->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef), UKismetSystemLibrary::StaticClass());
	ConvertToSoftObjectRef->AllocateDefaultPins();

	UK2Node_CallFunction* ConvertToObjectFunc = Compiler->SpawnIntermediateNode<UK2Node_CallFunction>(OriginNode, Graph);
	ConvertToObjectFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjectReferenceToObject), UKismetSystemLibrary::StaticClass());
	ConvertToObjectFunc->AllocateDefaultPins();
	*/
	// -----------------
	// Post-setup

	FString UniqueID = *Compiler.GetGuid(this);
	
	Node_SoftObjectPath->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath)));
	Node_SoftObjectRef->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef)));
	Node_ResolveObject->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjectReferenceToObject)));
	
	if (IsValid(CastTo))
	{
		Node_CastToType->TargetType = CastTo;
	}
	
	Builder.FinishDeferredNodes();
	
	// -----------------
	// Make connections
	
	UEdGraphPin* PathInput     = Node_SoftObjectPath->FindPin(FName("PathString"));
	UEdGraphPin* PathOutput    = Node_SoftObjectPath->GetReturnValuePin();
	UEdGraphPin* SoftRefInput  = Node_SoftObjectRef->FindPin(FName("SoftObjectPath"));
	UEdGraphPin* SoftRefOutput = Node_SoftObjectRef->GetReturnValuePin();
	UEdGraphPin* ConvertInput  = Node_ResolveObject->FindPin(FName("SoftObject"));
	UEdGraphPin* ConvertOutput = Node_ResolveObject->GetReturnValuePin();

	FString ActorPath = TargetActor.ToSoftObjectPath().ToString();
	Builder.SetDefaultValue(PathInput, ActorPath);
	
	Builder.CreateConnection(PathOutput, SoftRefInput);
	Builder.CreateConnection(SoftRefOutput, ConvertInput);
	//Builder.CopyExternalConnection(Node_This.FoundActor, ConvertOutput);
	
	if (IsValid(CastTo))
	{
		Builder.CreateConnection(ConvertOutput, Node_CastToType.ObjectToCast);
		Builder.CopyExternalConnection(Node_This.FoundActor, Node_CastToType.CastedObject);
	}
	
	// Done!
	if (!bIsErrorFree)
	{
		Compiler.MessageLog.Error(*LOCTEXT("InternalConnectionError", "Internal connection error. @@").ToString(), this);
	}
	
	ErrorState = bIsErrorFree ? EBangoFindActorNode_ErrorState::OK : EBangoFindActorNode_ErrorState::Error;
	
	// Disconnect ThisNode from the graph
	BreakAllNodeLinks();
}

void UK2Node_BangoFindActor::ExpandNode_ManualName(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph)
{
	Super::ExpandNode(Compiler, SourceGraph);

	const UEdGraphSchema_K2* Schema = Compiler.GetSchema();
	bool bIsErrorFree = true;

	namespace NB = BangoNodeBuilder;
	NB::Builder Builder(Compiler, SourceGraph, this, Schema, &bIsErrorFree, FVector2f(5, 5));
	
	// -----------------
	// Make nodes
	
	auto Node_This =					Builder.WrapExistingNode<NB::BangoFindActor>(this);
	auto Node_FindActorFunction	=		Builder.MakeNode<NB::CallFunction>(0, 1);
	auto Node_CastToType =				Builder.MakeNode<NB::DynamicCast_Pure>(1, 1);
	
	// -----------------
	// Post-setup

	Node_FindActorFunction->SetFromFunction(UBangoActorIDBlueprintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBangoActorIDBlueprintFunctionLibrary, K2_GetActorByName)));
	
	if (IsValid(CastTo))
	{
		Node_CastToType->TargetType = CastTo;
	}
	
	Builder.FinishDeferredNodes();
	
	// -----------------
	// Make connections
	
	if (Node_This.BangoName->HasAnyConnections())
	{
		Builder.CopyExternalConnection(Node_This.BangoName, Node_FindActorFunction.FindPin("Name"));
	}
	else
	{
		Builder.SetDefaultValue(Node_FindActorFunction.FindPin("Name"), Node_This.BangoName->DefaultValue);
	}
	
	if (IsValid(CastTo))
	{
		Builder.CreateConnection(Node_FindActorFunction->GetReturnValuePin(), Node_CastToType.ObjectToCast);
		Builder.CopyExternalConnection(Node_This.FoundActor, Node_CastToType.CastedObject);
	}
	else
	{
		Builder.CopyExternalConnection(Node_This.FoundActor, Node_FindActorFunction->GetReturnValuePin());
	}
	
	// Done!
	if (!bIsErrorFree)
	{
		Compiler.MessageLog.Error(*LOCTEXT("InternalConnectionError", "Internal connection error. @@").ToString(), this);
	}
	
	ErrorState = bIsErrorFree ? EBangoFindActorNode_ErrorState::OK : EBangoFindActorNode_ErrorState::Error;
	
	// Disconnect ThisNode from the graph
	BreakAllNodeLinks();
}

void UK2Node_BangoFindActor::SetActor(AActor* Actor)
{
	TSoftObjectPtr<AActor> ActorPath = Actor;
	
	ActorPath = Bango::Editor::UnfixPIEActorPath(ActorPath.ToString());
	
	TargetActor = ActorPath;
	
	CastTo = Actor->GetClass();

	// Manual name is unused with a target actor
	TargetName = NAME_None;
}

AActor* UK2Node_BangoFindActor::GetReferencedLevelActor() const
{
	if (TargetActor.IsPending())
	{
		UE_LOG(LogBango, Warning, TEXT("Actor is unloaded, can't jump!"));
	}
	
	else if (TargetActor.IsValid())
	{
		return TargetActor.Get();
	}
	
	return nullptr;
}

void UK2Node_BangoFindActor::JumpToDefinition() const
{
	AActor* Actor = TargetActor.Get();
	
	if (!Actor)
	{
		return;
	}

	// First clear the previous selection
	GEditor->GetSelectedActors()->Modify();
	GEditor->SelectNone( false, true );
	GEditor->SelectActor(Actor, true, true, false);

	// Execute the command to move camera to the object(s).
	GUnrealEd->Exec_Camera( TEXT("ALIGN ACTIVEVIEWPORTONLY"),*GLog); 
}

#undef LOCTEXT_NAMESPACE
