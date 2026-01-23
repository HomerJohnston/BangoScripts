#pragma once

#include "BangoScripts/Core/BangoScriptContainer.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"

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
	Unset,
	ContentAssetScript,
	LevelScript,
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

	TSharedPtr<SVerticalBox> Box;
	
	TMulticastDelegate<void()> PostScriptCreated;
	TMulticastDelegate<void()> PreScriptDeleted;
		
	FText ScriptNameText;
	
	EBangoScriptRenameStatus ProposedNameStatus;

	TArray<TSharedRef<IClassViewerFilter>> BangoScriptClassViewerFilters;

	// ------------------------------------------

	void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	int WidgetIndex_CreateDeleteScriptButtons() const;
	
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
	
	int WidgetIndex_GraphEditor() const;
	
	FReply OnClicked_EditScript() const;
	
	FReply OnClicked_EnlargeGraphView() const;
	
	FReply OnClicked_RenameScript() const;
	
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

	void OnPreScriptDeleted();	

	void UpdateBox();
	
	// ------------------------------------------

	AActor* GetOwnerActor() const;
	
	UObject* GetOuter() const;
	
	UBlueprint* GetBlueprint() const;
	
	UEdGraph* GetPrimaryEventGraph() const;
	
	TSubclassOf<UBangoScript> GetScriptClass() const;
	
	// ------------------------------------------
	
	void OnScriptContainerDestroyed(IBangoScriptHolderInterface& ScriptHolder);
	
	void OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap);
	
	void GetScriptContainerAndOuter(UObject*& Outer, FBangoScriptContainer*& ScriptContainer) const;
	
	IBangoScriptHolderInterface& GetScriptHolder() const;
	
	EBangoScriptType GetScriptType() const;
};