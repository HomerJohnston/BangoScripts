#include "SGraphPinClass_BangoScript.h"

#include "BangoScripts/Core/BangoScript.h"
#include "ClassViewerFilter/BangoClassViewerFilter.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBangoScriptClassViewer.h"
#include "Widgets/SBangoScriptPropertyEditorClass.h"

#define LOCTEXT_NAMESPACE "Bango"

class FPropertyEditorClassFilter;

class FPropertyEditorClassFilter : public IClassViewerFilter
{
public:
	/** The meta class for the property that classes must be a child-of. */
	const UClass* ClassPropertyMetaClass = nullptr;

	/** The interface that must be implemented. */
	const UClass* InterfaceThatMustBeImplemented = nullptr;

	/** Whether or not abstract classes are allowed. */
	bool bAllowAbstract = false;

	/** Filters all class types to be UVerseClass types with a <concrete> attribute */
	bool bRequiresVerseConcrete = false;

	/** Filters all class types to be UVerseClass types with a <castable> attribute */
	bool bRequiresVerseCastable = false;

	/** Classes that can be picked */
	TArray<const UClass*> AllowedClassFilters;

	/** Classes that can't be picked */
	TArray<const UClass*> DisallowedClassFilters;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		return IsClassAllowedHelper(InClass);
	}
	
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InBlueprint, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return IsClassAllowedHelper(InBlueprint);
	}

private:

	template <typename TClass>
	bool IsClassAllowedHelper(TClass InClass)
	{
		bool bMatchesFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated) &&
			(bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract));

		if (bMatchesFlags && InClass->IsChildOf(ClassPropertyMetaClass)
			&& (!InterfaceThatMustBeImplemented || InClass->ImplementsInterface(InterfaceThatMustBeImplemented)))
		{
			auto PredicateFn = [InClass](const UClass* Class)
			{
				return InClass->IsChildOf(Class);
			};

			if (DisallowedClassFilters.FindByPredicate(PredicateFn) == nullptr &&
				(AllowedClassFilters.Num() == 0 || AllowedClassFilters.FindByPredicate(PredicateFn) != nullptr))
			{
				return true;
			}
		}

		return false;
	}
};

TSharedRef<SWidget> SGraphPinClass_BangoScript::GenerateAssetPicker()
{
	/*
			return SNew(SBangoScriptPropertyEditorClass)
			.MetaClass(UBangoScript::StaticClass())
			.SelectedClass(this, &SGraphPinClass_BangoScript::SelectedClass_ScriptClass)
			.OnSetClass(this, &SGraphPinClass_BangoScript::OnSetClass_ScriptClass)
			.ToolTipText(LOCTEXT("ScriptClassSelector_ToolTipText", "Select a content script."));
	*/
	
	ClassViewerOptions.bShowBackgroundBorder = false;
	ClassViewerOptions.bShowUnloadedBlueprints = true;
	ClassViewerOptions.bShowNoneOption = true;

	//ClassViewerOptions.bIsBlueprintBaseOnly = bIsBlueprintBaseOnly;
	ClassViewerOptions.bIsPlaceableOnly = false;
	ClassViewerOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName; // : EClassViewerNameTypeToDisplay::ClassName);
	ClassViewerOptions.DisplayMode = EClassViewerDisplayMode::ListView;
	ClassViewerOptions.bAllowViewOptions = true;
	ClassViewerOptions.bShowObjectRootClass	= false;

	TSharedRef<FPropertyEditorClassFilter> PropEdClassFilter = MakeShared<FPropertyEditorClassFilter>();
	PropEdClassFilter->ClassPropertyMetaClass = UBangoScript::StaticClass();
	PropEdClassFilter->bAllowAbstract = false;
	//PropEdClassFilter->AllowedClassFilters = AllowedClassFilters;
	//PropEdClassFilter->DisallowedClassFilters = DisallowedClassFilters;

	ClassViewerOptions.ClassFilters.Add(PropEdClassFilter);

	ClassFilter = MakeShared<FBangoScriptsScriptContainerClassViewerFilter>(ClassViewerOptions);// FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassFilter(ClassViewerOptions);
	ClassFilterFuncs = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateFilterFuncs();
	
	FOnClassPicked OnPicked(FOnClassPicked::CreateSP(this, &SGraphPinClass_BangoScript::__OnPickedNewClass));
	
	return SNew(SBox)
	.WidthOverride(280.0f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.MaxHeight(500.0f)
		[ 
			SNew(SBorder)
			.Padding(4.0f)
			.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
			[
				SNew(SBangoScriptClassViewer, ClassViewerOptions)
				.OnClassPickedDelegate(OnPicked)
			]
		]
	];
}

const UClass* SGraphPinClass_BangoScript::SelectedClass_ScriptClass() const
{
	return nullptr;
}

void SGraphPinClass_BangoScript::__OnPickedNewClass(UClass* ChosenClass)
{
	OnPickedNewClass(ChosenClass);
}

#undef LOCTEXT_NAMESPACE
