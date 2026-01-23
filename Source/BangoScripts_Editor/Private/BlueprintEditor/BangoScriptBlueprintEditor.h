#pragma once

#include "BlueprintEditor.h"
#include "BangoScripts/Core/BangoScript.h"

class UBangoScriptBlueprint;
class UEdGraph;

class FBangoScriptBlueprintEditor : public FBlueprintEditor
{
public:
	FBangoScriptBlueprintEditor();
	
	virtual ~FBangoScriptBlueprintEditor();
	
public:
	void SetupGraphEditorEvents(UEdGraph* InGraph, SGraphEditor::FGraphEditorEvents& InEvents) override;
	
	/** Setup all the events that the graph editor can handle */
	void SetupGraphEditorEvents_Impl(UBlueprint* Blueprint, UEdGraph* InGraph, SGraphEditor::FGraphEditorEvents& InEvents);

	void Tick(float DeltaTime) override;
	//FActionMenuContent OnCreateGraphActionMenu_Impl(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);
	
	void OnDropActors(const TArray<TWeakObjectPtr<AActor>>& Actors, UEdGraph* EdGraph, const UE::Math::TVector2<float>& Vector2) const;
	
	// Sadly Epic didn't expose enough functionality for me to prevent adding multiple graphs 
	//bool AreEventGraphsAllowed() const override { return false; }
	FGraphAppearanceInfo GetGraphAppearance(UEdGraph* InGraph) const override;
	
	FText GetLevelNameAsText() const;
	
	FText GetOwnerNameAsText() const;

	float OpenTime = 0.0f;
	
	FText WarningText;
	
	static bool bDebugDrawRegistered;
	
	FDelegateHandle DebugDrawHandle; 
		
	void SetWarningText(const FText& InText);
	
	void InitBangoBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode);
	
	void AddEditingObject(UObject* Object) override;
	
	void SetCurrentMode(FName NewMode) override;
	
protected:
	void PostInitAssetEditor() override;
	
	void PasteGeneric() override;
	
	bool CanPasteGeneric() const override;
	
	bool CanPasteNodes() const override;
	
	void PasteNodesHere(UEdGraph* DestinationGraph, const FVector2f& GraphLocation) override;
	
	void OnBangoScriptRan(UBangoScript* ScriptInstance);
	
	void OnBangoScriptFinished(UBangoScript* ScriptInstance);
	
public:
	UBangoScriptBlueprint* GetBangoScriptBlueprintObj();
	
	const UBangoScriptBlueprint* GetBangoScriptBlueprintObj() const;
	
protected:
	// Copied from Editor.h
	bool GetBEGIN(const TCHAR** Stream, const TCHAR* Match) const;
	bool GetEND(const TCHAR** Stream, const TCHAR* Match) const;
};
