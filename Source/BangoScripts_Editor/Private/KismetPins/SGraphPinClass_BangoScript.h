#pragma once

#include "ClassViewerModule.h"
#include "KismetPins/SGraphPinClass.h"

class SGraphPinClass_BangoScript : public SGraphPinClass
{
	TSharedRef<SWidget> GenerateAssetPicker() override;
	
	const UClass* SelectedClass_ScriptClass() const;
	
	void __OnPickedNewClass(UClass* ChosenClass);
	
	/** Class filter that the class viewer is using. */
	TSharedPtr<IClassViewerFilter> ClassFilter;

	/** Filter functions for class viewer. */
	TSharedPtr<FClassViewerFilterFuncs> ClassFilterFuncs;

	/** Options used for creating the class viewer. */
	FClassViewerInitializationOptions ClassViewerOptions;
};
