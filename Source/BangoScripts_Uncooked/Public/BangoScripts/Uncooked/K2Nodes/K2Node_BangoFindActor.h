#pragma once

#include "BangoScripts/Uncooked/K2Nodes/Base/_K2NodeBangoBase.h"
#include "BangoScripts/Uncooked/NodeBuilder/BangoNodeBuilder.h"

#include "K2Node_BangoFindActor.generated.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

enum class EBangoFindActorNode_ErrorState : uint8
{
	OK,
	Error,
};

/**
 * Works as either a soft pointer to an actor (when dragged onto the graph from world outliner) or a manual Bango ID name lookup.
 */
UCLASS(MinimalAPI, DisplayName = "FindActor")
class UK2Node_BangoFindActor : public UK2Node_BangoBase
{
	GENERATED_BODY()

public:
	UK2Node_BangoFindActor();

protected:	
	/** This node will *either* target a manual name with CastTo, OR it will use TargetActor below. */
	UPROPERTY(VisibleAnywhere)
	FName TargetName;
	
	/** Change to the desired class to cast the output automatically. */
	UPROPERTY(EditAnywhere, meta = (EditCondition = "TargetName != NAME_None", EditConditionHides))
	TSubclassOf<AActor> CastTo;
	
	/** Level-blueprint-like actor reference, used by dragging an actor onto the blueprint from the world outliner. */
	UPROPERTY(VisibleAnywhere)
	TSoftObjectPtr<AActor> TargetActor;
	
	/** Used by the slate widget to highlight the node. */
	EBangoFindActorNode_ErrorState ErrorState;

	UPROPERTY(EditAnywhere, DuplicateTransient)
	FLinearColor Color;
	
public:
	TSubclassOf<AActor> GetCastTo() const { return CastTo; }
	
	TSoftObjectPtr<AActor> GetTargetActor() const { return TargetActor; } 
	
	const FLinearColor& GetColor() const { return Color; }
	
	bool ShouldDrawCompact() const override;
	
	FLinearColor GetNodeBodyTintColor() const override;
	
	FLinearColor GetNodeTitleColor() const override;
	
	FLinearColor GetNodeTitleTextColor() const override;
	
	EBangoFindActorNode_ErrorState GetErrorState() const { return ErrorState; }
	
	FText GetTooltipText() const override;
	
public:
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	virtual void PostPlacedNewNode() override;
	
	void AllocateDefaultPins() override;
	
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	
	void ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph) override;
	
	void ExpandNode_SoftActor(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph);
	
	void ExpandNode_ManualName(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph);
	
	void FixUpForNewOwnerActor(const TSoftObjectPtr<AActor>& OldOwner, const TSoftObjectPtr<AActor>& NewOwner) override;
	
	bool IsNodePure() const override { return true; }
	
	BANGOSCRIPTS_UNCOOKED_API void SetActor(AActor* Actor);
	
	AActor* GetReferencedLevelActor() const override;
	
	void JumpToDefinition() const override;
	
	// Intentionally unreflected; slate widgets will update this whenever the node is selected. The ScriptComponent visualizer will use it to highlight recently selected connections.
	uint64 LastSelectedFrame = 0;
};

using namespace BangoNodeBuilder;

// ==========================================
MAKE_NODE_TYPE(BangoFindActor, UK2Node_BangoFindActor, NORMAL_CONSTRUCTION, BangoName, BangoGuid, FoundActor, TargetActor);

inline void BangoFindActor::Construct()
{
	AllocateDefaultPins();
	TargetActor = FindPin("SoftActor");
	BangoName = FindPin("BangoName");
	BangoGuid = FindPin("BangoGuid");
	FoundActor = FindPin("FoundActor");
}
#undef LOCTEXT_NAMESPACE