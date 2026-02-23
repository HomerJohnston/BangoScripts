#pragma once

#include "SGraphNode.h"
#include "KismetNodes/SGraphNodeK2Base.h"

class UK2Node_BangoFindActor;
class FBangoFindActorAction;

class SGraphNode_BangoFindActor : public SGraphNodeK2Base
{
	enum class EHardActorReferenceStatus
	{
		Unknown,
		HardReference,
		SoftReference,
	};
	
public:
	SLATE_BEGIN_ARGS(SGraphNode_BangoFindActor)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UEdGraphNode* InNode);

	TSharedRef<SWidget> CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle) override;
	
	void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;
	
	void CreateBelowWidgetControls(TSharedPtr<SVerticalBox> MainBox) override;
	
	void CreatePinWidgets() override;

	TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;

	void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	
	TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const override;
	
	UK2Node_BangoFindActor* GetBangoFindActorNode() const;
	
	void UpdateGraphNode() override;

	void UpdateCompactNode();

	FText GetNodeCompactTitle_Impl() const;

	FSlateColor ColorAndOpacity_ActorLabel() const;
	
	EVisibility Visibility_BangoNameIndicator() const;
	
	EVisibility Visibility_UnloadedIndicator() const;
	
	FText Text_BangoNameIndicator() const;

	FSlateColor ColorAndOpacity_SoftIndicator() const;
	
	EHardActorReferenceStatus GetHardActorReferenceState() const;
	
	FReply OnClicked_ToggleHard();
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	const TSoftObjectPtr<AActor> GetTargetActorSoft() const;
};
