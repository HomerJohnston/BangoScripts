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
	
	/** Change to the desired class to cast the output automatically. */
	UPROPERTY(Category = "Default", EditAnywhere)
	TSubclassOf<AActor> CastTo = nullptr;
	
	/** Level-blueprint-like actor reference, used by dragging an actor onto the blueprint from the world outliner. */
	UPROPERTY(Category = "Default", VisibleAnywhere, DisplayName = "Actor")
	TSoftObjectPtr<AActor> TargetActor = nullptr;
	
	/** This is only used for copy/paste/new node detection */
	UPROPERTY()
	bool bInitialized = false;
	
	/** Used by the slate widget to highlight the node. */
	EBangoFindActorNode_ErrorState ErrorState;
	
public:
	TSubclassOf<AActor> GetCastTo() const { return CastTo; }
	
	TSoftObjectPtr<AActor> GetTargetActor() const { return TargetActor; } 
	
	bool ShouldDrawCompact() const override;
	
	FLinearColor GetNodeBodyTintColor() const override;
	
	FLinearColor GetNodeTitleColor() const override;
	
	FLinearColor GetNodeTitleTextColor() const override;
	
	EBangoFindActorNode_ErrorState GetErrorState() const { return ErrorState; }
	
	FText GetTooltipText() const override;
	
public:
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	void PostPlacedNewNode() override;
	
	void PostPasteNode() override;
	
	void InitializeInternal();
	
	void DestroyNode() override;
	
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
	
	BANGOSCRIPTS_UNCOOKED_API void ToggleHard();

	// Intentionally unreflected; slate widgets will update this whenever the node is selected. The ScriptComponent visualizer will use it to highlight recently selected connections.
	uint64 LastSelectedFrame = 0;
	
	bool CanEditChange(const FProperty* InProperty) const override;
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