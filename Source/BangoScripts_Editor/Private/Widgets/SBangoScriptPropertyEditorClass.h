// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Misc/Attribute.h"
#include "Fonts/SlateFontInfo.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboButton.h"
#include "Styling/AppStyle.h"
#include "Presentation/PropertyEditor/PropertyEditor.h"
#include "UserInterface/PropertyEditor/PropertyEditorConstants.h"
#include "PropertyCustomizationHelpers.h"
#include "ClassViewerModule.h"

class IClassViewerFilter;
class FClassViewerFilterFuncs;

class BangoPropertyEditorConstants
{
public:

	static const FName PropertyFontStyle;
	static const FName CategoryFontStyle;

	static const FName MD_Bitmask;
	static const FName MD_BitmaskEnum;
	static const FName MD_UseEnumValuesAsMaskValuesInEditor;

	static constexpr float PropertyRowHeight = 26.0f;

	static const FText DefaultUndeterminedText;

	static const FSlateBrush* GetOverlayBrush(const TSharedRef<class FPropertyEditor> PropertyEditor);

	static FSlateColor GetRowBackgroundColor(int32 IndentLevel, bool IsHovered);
};

// Based on SPropertyEditorClass

/**
 * Epic always uses the global IClassViewerFilter filters, even when manually building SClassPropertyEntryBoxes yourself and specifying your own filters. 
 * So I need to completely make my own script class picker just to control the filter, yay winning!
 */
class SBangoScriptPropertyEditorClass : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPropertyEditorClass)
		: _Font(FAppStyle::GetFontStyle(BangoPropertyEditorConstants::PropertyFontStyle)) 
		, _MetaClass(UObject::StaticClass())
		, _ShowDisplayNames(false)
		, _InvalidObjectDisplayText("None")
		{}
		SLATE_ARGUMENT(FSlateFontInfo, Font)

		/** Arguments used when constructing this outside of a PropertyEditor (PropertyEditor == null), ignored otherwise */
		/** The meta class that the selected class must be a child-of (required if PropertyEditor == null) */
		SLATE_ARGUMENT(const UClass*, MetaClass)
		/** Attribute used to get the currently selected class (required if PropertyEditor == null) */
		SLATE_ATTRIBUTE(const UClass*, SelectedClass)
		/** Should we show the view options button at the bottom of the class picker?*/
		SLATE_ARGUMENT(bool, ShowViewOptions)
		/** Should we prettify class names on the class picker? (ie show their display name) */
		SLATE_ARGUMENT(bool, ShowDisplayNames)
		/** Delegate used to set the currently selected class (required if PropertyEditor == null) */
		SLATE_EVENT(FOnSetClass, OnSetClass)
		/** Text to show when no class is selected (Default of "None") */
		SLATE_ARGUMENT(FString, InvalidObjectDisplayText)
	SLATE_END_ARGS()

	static bool Supports(const TSharedRef< class FPropertyEditor >& InPropertyEditor);

	void Construct(const FArguments& InArgs, const TSharedPtr< class FPropertyEditor >& InPropertyEditor = nullptr);

	void GetDesiredWidth(float& OutMinDesiredWidth, float& OutMaxDesiredWidth);

private:
	void SendToObjects(const FString& NewValue);

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	TSharedRef<SWidget> ConstructClassViewer();

	/** 
	 * Generates a class picker with a filter to show only classes allowed to be selected. 
	 *
	 * @return The Class Picker widget.
	 */
	TSharedRef<SWidget> GenerateClassPicker();

	/** 
	 * Callback function from the Class Picker for when a Class is picked.
	 *
	 * @param InClass			The class picked in the Class Picker
	 */
	void OnClassPicked(UClass* InClass);

	/**
	 * Gets the active display value as a string
	 */
	FText GetDisplayValueAsString() const;

	bool CanEdit() const;

private:
	/** The property editor we were constructed for, or null if we're editing using the construction arguments */
	TSharedPtr<class FPropertyEditor> PropertyEditor;

	/** Used when the property deals with Classes and will display a Class Picker. */
	TSharedPtr<class SComboButton> ComboButton;

	/** Class filter that the class viewer is using. */
	TSharedPtr<IClassViewerFilter> ClassFilter;

	/** Filter functions for class viewer. */
	TSharedPtr<FClassViewerFilterFuncs> ClassFilterFuncs;

	/** Options used for creating the class viewer. */
	FClassViewerInitializationOptions ClassViewerOptions;

	/** The meta class that the selected class must be a child-of */
	const UClass* MetaClass;
	/** Should we show the view options button at the bottom of the class picker?*/
	bool bShowViewOptions;
	/** Should we prettify class names on the class picker? (ie show their display name) */
	bool bShowDisplayNames;
	/** Text which is displayed instead of the normal class-text (used to show 'Set to Value' on optionals) */
	FString InvalidObjectDisplayText;

	/** Attribute used to get the currently selected class (required if PropertyEditor == null) */
	TAttribute<const UClass*> SelectedClass;
	/** Delegate used to set the currently selected class (required if PropertyEditor == null) */
	FOnSetClass OnSetClass;

	void CreateClassFilter(const TArray<TSharedRef<IClassViewerFilter>>& InClassFilters);
};
