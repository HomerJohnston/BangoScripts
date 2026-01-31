#pragma once

#include "BangoScripts/Core/BangoScriptContainer.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"

class SHorizontalBox;
class IClassViewerFilter;
struct FCanLoadMap;
class SVerticalBox;
class FBangoScriptBlueprintEditor;

enum class EBangoScriptRenameStatus : uint8
{
	ValidNewName,
	MatchesCurrent,
	MatchesOther,
	InvalidNewName,
};

enum class EBangoScriptType : uint8
{
	None,
	ContentAssetScript,
	LevelScript,
};

enum class EBangoScriptStatus : uint8
{
	None,
	Saved,
	Unsaved
};

class FBangoScriptContainerCustomization : public IPropertyTypeCustomization
{
public:
	FBangoScriptContainerCustomization();
	
	~FBangoScriptContainerCustomization();
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	// ------------------------------------------
	
protected:
	TSharedPtr<IPropertyHandle> ScriptContainerProperty;
	TSharedPtr<IPropertyHandle> ScriptClassProperty;
	TSharedPtr<IPropertyHandle> GuidProperty;
	TWeakObjectPtr<UEdGraph> CurrentGraph;

	TSharedPtr<SHorizontalBox> HeaderNameContent;
	TSharedPtr<SHorizontalBox> HeaderValueContent;
	TSharedPtr<SVerticalBox> GraphWidgetBox;
	
	TMulticastDelegate<void()> PostScriptCreated;
	TMulticastDelegate<void()> PreScriptDeleted;
		
	FText ScriptNameText;
	
	EBangoScriptRenameStatus ProposedNameStatus;

	TArray<TSharedRef<IClassViewerFilter>> BangoScriptClassViewerFilters;
	
	// ------------------------------------------

	void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	FReply OnClicked_CreateScript();

	FText Text_UnsetDeleteScript() const;
	
	FReply OnClicked_UnsetDeleteScript();
	
	bool IsEnabled_DeleteUnsetButton() const;
	
	bool IsEnabled_ScriptClassPicker() const;
	
	bool IsEnabled_CreateLevelScriptButton() const;
	
	void OnSetClass_ScriptClass(const UClass* Class) const;
	
	const UClass* SelectedClass_ScriptClass() const;
	
	// ------------------------------------------

	void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	
	EVisibility Visibility_HasValidGraph() const;
	
	EVisibility Visibility_HasNoValidGraph() const;
	
	EVisibility Visibility_HasScriptInputs() const;
	
	EVisibility Visibility_DeleteUnsetButton() const;
	
	FReply OnClicked_EditScript() const;
	
	FReply OnClicked_EnlargeGraphView() const;
	
	FText Text_RefreshScriptInputs() const;
	
	bool IsEnabled_RefreshScriptInputs() const;
	
	FSlateColor ButtonColorAndOpacity_RefreshScriptInputs() const;
	
	FReply OnClicked_RefreshScriptInputs() const;
	
	void OnTextChanged_ScriptNameEditableText(const FText& Text);
	
	void SetProposedScriptName(const FText& Text);
	
	EBangoScriptRenameStatus GetProposedNameStatus();
	
	FSlateColor ForegroundColor_ScriptNameEditableText() const;
	
	FSlateColor FocusedForegroundColor_ScriptNameEditableText() const;
	
	bool IsEnabled_RenameScriptButton() const;
	
	TSharedRef<SWidget> GetPopoutGraphEditor(FVector2D WindowSize) const;
	
	// ------------------------------------------

	void OnPostScriptCreatedOrRenamed();

	void OnPreScriptUnsetOrDeleted();	

	void UpdateHeaderRowNameAndValueContent();
	
	void UpdateGraphWidgetBox();
	
	// ------------------------------------------

	AActor* GetOwnerActor() const;
	
	UObject* GetOuter() const;
	
	UBlueprint* GetBlueprint() const;
	
	UEdGraph* GetPrimaryEventGraph() const;
	
	bool HasScriptClassAssigned() const;
	
	TSubclassOf<UBangoScript> GetScriptClass() const;
	
	bool IsScriptClassStale() const;
	
	FString GetScriptClassPath() const;
	
	// ------------------------------------------
	
	void OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap);
	
	void GetScriptContainerAndOuter(UObject*& Outer, FBangoScriptContainer*& ScriptContainer) const;
	
	IBangoScriptHolderInterface& GetScriptHolder() const;
	
	EBangoScriptStatus GetScriptStatus() const;
	
	EBangoScriptType GetScriptType() const;
	
	void SendDummyPECPEvent(UObject* Object = nullptr) const;
};