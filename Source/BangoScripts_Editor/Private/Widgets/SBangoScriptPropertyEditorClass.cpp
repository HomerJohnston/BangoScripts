// Copyright Epic Games, Inc. All Rights Reserved.

#include "SBangoScriptPropertyEditorClass.h"
#include "Engine/Blueprint.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBox.h"
#include "DragAndDrop/ClassDragDropOp.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "SBangoScriptClassViewer.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "ClassViewerFilter/BangoClassViewerFilter.h"
#include "UObject/Object.h"
#include "UObject/Class.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

const FName BangoPropertyEditorConstants::PropertyFontStyle( TEXT("PropertyWindow.NormalFont") );
const FName BangoPropertyEditorConstants::CategoryFontStyle( TEXT("DetailsView.CategoryFontStyle") );

const FName BangoPropertyEditorConstants::MD_Bitmask( TEXT("Bitmask") );
const FName BangoPropertyEditorConstants::MD_BitmaskEnum( TEXT("BitmaskEnum") );
const FName BangoPropertyEditorConstants::MD_UseEnumValuesAsMaskValuesInEditor( TEXT("UseEnumValuesAsMaskValuesInEditor") );

const FText BangoPropertyEditorConstants::DefaultUndeterminedText(NSLOCTEXT("PropertyEditor", "MultipleValues", "Multiple Values"));

// TODO duplicated code between here and SGraphPinClass_BangoScript.cpp should be unified
namespace BangoScriptPropertyEditorClass
{
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
}

void SBangoScriptPropertyEditorClass::GetDesiredWidth(float& OutMinDesiredWidth, float& OutMaxDesiredWidth)
{
	OutMinDesiredWidth = 200.0f;
	OutMaxDesiredWidth = 400.0f;
}

void SBangoScriptPropertyEditorClass::Construct(const FArguments& InArgs, const TSharedPtr< FPropertyEditor >& InPropertyEditor)
{
	check(InArgs._MetaClass);
	check(InArgs._SelectedClass.IsSet());
	check(InArgs._OnSetClass.IsBound());

	MetaClass = InArgs._MetaClass;
	bShowViewOptions = InArgs._ShowViewOptions;
	bShowDisplayNames = InArgs._ShowDisplayNames;
	SelectedClass = InArgs._SelectedClass;
	OnSetClass = InArgs._OnSetClass;

	InvalidObjectDisplayText = InArgs._InvalidObjectDisplayText;

	CreateClassFilter();

	SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &SBangoScriptPropertyEditorClass::GenerateClassPicker)
		.ToolTipText(this, &SBangoScriptPropertyEditorClass::GetDisplayValueAsString)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &SBangoScriptPropertyEditorClass::GetDisplayValueAsString)
			.Font(InArgs._Font)
		];

	ChildSlot
	[
		ComboButton.ToSharedRef()
	];
}

/** Util to give better names for BP generated classes */
static FText GetClassDisplayName(const UObject* Object, bool bShowDisplayNames, const FString& InvalidObjectDisplayText)
{
	const UClass* Class = Cast<UClass>(Object);
	if (Class != nullptr)
	{
		if (bShowDisplayNames)
		{
			return Class->GetDisplayNameText();
		}
		
		UBlueprint* BP = UBlueprint::GetBlueprintFromClass(Class);
		if(BP != nullptr)
		{
			return FText::FromString(BP->GetName());
		}
	}
	return (Object) ? FText::FromString(Object->GetName()) : FText::FromString(InvalidObjectDisplayText);
}

FText SBangoScriptPropertyEditorClass::GetDisplayValueAsString() const
{
	static bool bIsReentrant = false;

	// Guard against re-entrancy which can happen if the delegate executed below (SelectedClass.Get()) forces a slow task dialog to open, thus causing this to lose context and regain focus later starting the loop over again
	if( !bIsReentrant )
	{
		TGuardValue<bool> Guard( bIsReentrant, true );

		return GetClassDisplayName(SelectedClass.Get(), bShowDisplayNames, InvalidObjectDisplayText);
	}
	else
	{
		return FText::GetEmpty();
	}
}

void SBangoScriptPropertyEditorClass::CreateClassFilter()
{
	ClassViewerOptions.bShowBackgroundBorder = false;
	ClassViewerOptions.bShowUnloadedBlueprints = true;
	ClassViewerOptions.bShowNoneOption = true;

	//ClassViewerOptions.bIsBlueprintBaseOnly = bIsBlueprintBaseOnly;
	ClassViewerOptions.bIsPlaceableOnly = false;
	ClassViewerOptions.NameTypeToDisplay = (bShowDisplayNames ? EClassViewerNameTypeToDisplay::DisplayName : EClassViewerNameTypeToDisplay::ClassName);
	ClassViewerOptions.DisplayMode = EClassViewerDisplayMode::ListView;
	ClassViewerOptions.bAllowViewOptions = bShowViewOptions;
	ClassViewerOptions.bShowObjectRootClass	= false;

	TSharedRef<BangoScriptPropertyEditorClass::FPropertyEditorClassFilter> PropEdClassFilter = MakeShared<BangoScriptPropertyEditorClass::FPropertyEditorClassFilter>();
	PropEdClassFilter->ClassPropertyMetaClass = MetaClass;
	PropEdClassFilter->bAllowAbstract = false;
	//PropEdClassFilter->AllowedClassFilters = AllowedClassFilters;
	//PropEdClassFilter->DisallowedClassFilters = DisallowedClassFilters;

	ClassViewerOptions.ClassFilters.Add(PropEdClassFilter);

	ClassFilter = MakeShared<FBangoScriptsScriptContainerClassViewerFilter>(ClassViewerOptions);// FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassFilter(ClassViewerOptions);
	ClassFilterFuncs = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateFilterFuncs();
}

TSharedRef<SWidget> SBangoScriptPropertyEditorClass::GenerateClassPicker()
{
	FOnClassPicked OnPicked(FOnClassPicked::CreateSP(this, &SBangoScriptPropertyEditorClass::OnClassPicked));

	return SNew(SBox)
		.WidthOverride(280.0f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(500.0f)
			[
				SNew(SBangoScriptClassViewer, ClassViewerOptions)
				.OnClassPickedDelegate(OnPicked)
				//FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(ClassViewerOptions, OnPicked)
			]			
		];
}

void SBangoScriptPropertyEditorClass::OnClassPicked(UClass* InClass)
{
	if (!InClass)
	{
		SendToObjects(TEXT("None"));
	}
	else
	{
		SendToObjects(InClass->GetPathName());
	}

	ComboButton->SetIsOpen(false);
}

void SBangoScriptPropertyEditorClass::SendToObjects(const FString& NewValue)
{
	if (!NewValue.IsEmpty() && NewValue != TEXT("None"))
	{
		const UClass* NewClass = nullptr;

		if (const UObject* Object = StaticLoadObject(nullptr, nullptr, *NewValue))
		{
			if (const UClass* Class = Cast<UClass>(Object))
			{
				NewClass = Class;
			}
			else
			{
				UE_LOG(LogBangoEditor, Error, TEXT("Found object '%s' instead of a class."), *Object->GetFullName());
			}
		}
		else
		{
			UE_LOG(LogBangoEditor, Error, TEXT("Unable to load class '%s'."), *NewValue);
		}

		OnSetClass.Execute(NewClass);
	}
	else
	{
		OnSetClass.Execute(nullptr);
	}
}

static UObject* LoadDragDropObject(TSharedPtr<FAssetDragDropOp> UnloadedClassOp)
{
	FString AssetPath;

	// Find the class/blueprint path
	if (UnloadedClassOp->HasAssets())
	{
		AssetPath = UnloadedClassOp->GetAssets()[0].GetObjectPathString();
	}
	else if (UnloadedClassOp->HasAssetPaths())
	{
		AssetPath = UnloadedClassOp->GetAssetPaths()[0];
	}

	// Check to see if the asset can be found, otherwise load it.
	UObject* Object = FindObject<UObject>(nullptr, *AssetPath);
	if (Object == nullptr)
	{
		// Load the package.
		GWarn->BeginSlowTask(LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false);

		Object = LoadObject<UObject>(nullptr, *AssetPath);

		GWarn->EndSlowTask();
	}

	return Object;
}

void SBangoScriptPropertyEditorClass::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAssetDragDropOp> UnloadedClassOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (UnloadedClassOp.IsValid())
	{
		const UObject* Object = LoadDragDropObject(UnloadedClassOp);

		bool bOK = false;

		if (const UClass* Class = Cast<UClass>(Object))
		{
			bOK = ClassFilter->IsClassAllowed(ClassViewerOptions, Class, ClassFilterFuncs.ToSharedRef());
		}
		else if (const UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			if (Blueprint->GeneratedClass)
			{
				bOK = ClassFilter->IsClassAllowed(ClassViewerOptions, Blueprint->GeneratedClass, ClassFilterFuncs.ToSharedRef());
			}
		}
		
		if (bOK)
		{
			UnloadedClassOp->SetToolTip(FText::GetEmpty(), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")));
		}
		else
		{
			UnloadedClassOp->SetToolTip(FText::GetEmpty(), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error")));
		}
	}
}

void SBangoScriptPropertyEditorClass::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAssetDragDropOp> UnloadedClassOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (UnloadedClassOp.IsValid())
	{
		UnloadedClassOp->ResetToDefaultToolTip();
	}
}

FReply SBangoScriptPropertyEditorClass::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FClassDragDropOp> ClassOperation = DragDropEvent.GetOperationAs<FClassDragDropOp>();
	if (ClassOperation.IsValid())
	{
		// We can only drop one item into the combo box, so drop the first one.
		const FString ClassPath = ClassOperation->ClassesToDrop[0]->GetPathName();

		// Set the property, it will be verified as valid.
		SendToObjects(ClassPath);

		return FReply::Handled();
	}
	
	TSharedPtr<FAssetDragDropOp> UnloadedClassOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (UnloadedClassOp.IsValid())
	{
		FString AssetPath;

		// Find the class/blueprint path
		if (UnloadedClassOp->HasAssets())
		{
			AssetPath = UnloadedClassOp->GetAssets()[0].GetObjectPathString();
		}
		else if (UnloadedClassOp->HasAssetPaths())
		{
			AssetPath = UnloadedClassOp->GetAssetPaths()[0];
		}

		// Check to see if the asset can be found, otherwise load it.
		UObject* Object = FindObject<UObject>(nullptr, *AssetPath);
		if(Object == nullptr)
		{
			// Load the package.
			GWarn->BeginSlowTask(LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false);

			Object = LoadObject<UObject>(nullptr, *AssetPath);

			GWarn->EndSlowTask();
		}

		if (const UClass* Class = Cast<UClass>(Object))
		{
			if (ClassFilter->IsClassAllowed(ClassViewerOptions, Class, ClassFilterFuncs.ToSharedRef()))
			{
				// This was pointing to a class directly
				SendToObjects(Class->GetPathName());
			}
		}
		else if (const UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			if (Blueprint->GeneratedClass)
			{
				if (ClassFilter->IsClassAllowed(ClassViewerOptions, Blueprint->GeneratedClass, ClassFilterFuncs.ToSharedRef()))
				{
					// This was pointing to a blueprint, get generated class
					SendToObjects(Blueprint->GeneratedClass->GetPathName());
				}
			}
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
