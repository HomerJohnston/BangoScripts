#pragma once

#include "GraphEditor.h"

class FBlueprintEditor;
class FBangoScriptBlueprintEditor;

class SBangoGraphEditor : public SGraphEditor
{
	//SLATE_DECLARE_WIDGET_API(SBangoGraphEditor, SGraphEditor, BANGOSCRIPTS_EDITOR_API);
	
	SLATE_BEGIN_ARGS(SBangoGraphEditor) {}
		SLATE_ARGUMENT(TSharedPtr<FBlueprintEditor>, BlueprintEditor)

		SLATE_ATTRIBUTE( bool, IsEditable )		
		SLATE_ARGUMENT( UEdGraph*, GraphToEdit )
		SLATE_ARGUMENT( FGraphEditorEvents, GraphEvents)
		SLATE_ATTRIBUTE(bool, ShowGraphStateOverlay)
		
	SLATE_END_ARGS()
	
	TSharedPtr<FBlueprintEditor> BlueprintEditor;
	
	void Construct(const FArguments& InArgs);
};