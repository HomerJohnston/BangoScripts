#pragma once
#include "BangoScripts/Uncooked/K2Nodes/Base/_K2NodeBangoBase.h"

#include "K2Node_BangoSleep.generated.h"

class UK2Node_TemporaryVariable;

#define LOCTEXT_NAMESPACE "BangoScripts"

UCLASS(MinimalAPI, DisplayName = "Wait")
class UK2Node_BangoSleep : public UK2Node_BangoBase
{
	GENERATED_BODY()

public:
	UK2Node_BangoSleep();

protected:
	
	/**  */
	UPROPERTY()
	float Duration = 1.0f;
	
	/** If set, this node can only be skipped or cancelled. */
	UPROPERTY(EditAnywhere, Category = "Time", DisplayName = "Non-timed")
	bool bInfiniteDuration;
	
	/** Skipping will immediately run the output pin. */
	UPROPERTY(EditAnywhere, Category = "Controls", DisplayName = "Enable Skip Exec Pin")
	bool bEnableSkipExecPin;
	
	/** Cancelling will prevent this node from ever executing its output pin. */
	UPROPERTY(EditAnywhere, Category = "Controls", DisplayName = "Enable Cancel Exec Pin", meta = (EditCondition = "!bInfiniteDuration", EditConditionHides))
	bool bEnableCancelExecPin;

	/** Skipping will immediately run the output pin. Condition will be checked on tick! */
	UPROPERTY(EditAnywhere, Category = "Controls", DisplayName = "Enable Skip Condition Pin")
	bool bEnableSkipConditionPin;
	
	/** Cancelling will prevent this node from ever executing its output pin. Condition will be checked on tick! */
	UPROPERTY(EditAnywhere, Category = "Controls", DisplayName = "Enable Cancel Condition Pin", meta = (EditCondition = "!bInfiniteDuration", EditConditionHides))
	bool bEnableCancelConditionPin;

	/** Pausing will prevent this node's timer from counting down. Condition will be updated on tick! */
	UPROPERTY(EditAnywhere, Category = "Controls", DisplayName = "Enable Pause Condition Pin", meta = (EditCondition = "!bInfiniteDuration", EditConditionHides))
	bool bEnablePauseConditionPin;
	
public:
	bool IsInfiniteDuration() const { return bInfiniteDuration; }

	bool IsSkipExecPinEnabled() const { return bEnableSkipExecPin; }
	
	bool IsCancelExecPinEnabled() const { return !bInfiniteDuration && bEnableCancelExecPin; }
	
	bool IsSkipConditionPinEnabled() const { return bEnableSkipConditionPin; }
	
	bool IsCancelConditionPinEnabled() const { return !bInfiniteDuration && bEnableCancelConditionPin; }
	
	bool IsPauseConditionPinEnabled() const { return !bInfiniteDuration && bEnablePauseConditionPin; }
	
protected:
	
	UPROPERTY()
	bool bSkipExecTriggered;

	UPROPERTY()
	bool bCancelExecTriggered;
	
	UPROPERTY()
	double FinishTime;
	
public:
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	void AllocateDefaultPins() override;

	void ExpandNode(class FKismetCompilerContext& Compiler, UEdGraph* SourceGraph) override;	

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	FLinearColor GetNodeTitleColor() const override;
	
	FLinearColor GetNodeTitleTextColor() const override;
	
public:
	bool IsLatentForMacros() const override
	{
		return true;
	}

	FText GetToolTipHeading() const override
	{
		return LOCTEXT("LatentFunc", "Latent");
	}
};

#undef LOCTEXT_NAMESPACE