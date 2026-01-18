#include "SBangoScriptPickerDialog.h"

#include "Misc/MessageDialog.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Components/ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Editor/UnrealEdEngine.h"
#include "Editor.h"
#include "SBangoScriptClassViewer.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "SClassViewer.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "SPrimaryButton.h"
#include "UObject/Class.h"


void SBangoScriptPickerDialog::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	bAllowNone = InArgs._Options.bShowNoneOption;

	bPressedOk = false;
	ChosenClass = NULL;

	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	if (InArgs._Options.bShowClassesViewer)
	{
		//ClassViewer = StaticCastSharedRef<SClassViewer>(ClassViewerModule.CreateClassViewer(InArgs._Options, FOnClassPicked::CreateSP(this, &SBangoScriptPickerDialog::OnClassPicked)));
		
		ClassViewer = SNew( SBangoScriptClassViewer, InArgs._Options )
		.OnClassPickedDelegate(FOnClassPicked::CreateSP(this, &SBangoScriptPickerDialog::OnClassPicked));
	}

	bool bExpandCustomClassPicker = true;
	if (ClassViewer)
	{
		GConfig->GetBool(TEXT("/Script/UnrealEd.UnrealEdOptions"), TEXT("bExpandCustomClassPickerClassList"), bExpandCustomClassPicker, GEditorIni);
	}

	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	if (ClassViewer)
	{
		FMargin Padding = FMargin();
		Container->AddSlot()
		.AutoHeight()
		.Padding(Padding)
		[
			SNew(SExpandableArea)
			.BorderImage(FStyleDefaults::GetNoBrush())
			.BodyBorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			.HeaderPadding(FMargin(5.0f, 3.0f))
			.AllowAnimatedTransition(false)
			.MaxHeight(320.f)
			.InitiallyCollapsed(!bExpandCustomClassPicker)
			.OnAreaExpansionChanged(this, &SBangoScriptPickerDialog::OnCustomAreaExpansionChanged)
			.HeaderContent()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SBangoScriptPickerDialog", "AllClassesAreaTitle", "All Classes"))
				.TextStyle(FAppStyle::Get(), "ButtonText")
				.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
				.TransformPolicy(ETextTransformPolicy::ToUpper)
			]
			.BodyContent()
			[
				ClassViewer.ToSharedRef()
			]
		];
	}

	Container->AddSlot()
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Bottom)
	.Padding(8.f)
	[
		SNew(SUniformGridPanel)
		.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
		+SUniformGridPanel::Slot(0,0)
		[
			SNew(SPrimaryButton)
			.Text(NSLOCTEXT("SBangoScriptPickerDialog", "ClassPickerSelectButton", "Select"))
			.Visibility( this, &SBangoScriptPickerDialog::GetSelectButtonVisibility )
			.OnClicked(this, &SBangoScriptPickerDialog::OnClassPickerConfirmed)
		]
		+SUniformGridPanel::Slot(1,0)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("SBangoScriptPickerDialog", "ClassPickerCancelButton", "Cancel"))
			.HAlign(HAlign_Center)
			.OnClicked(this, &SBangoScriptPickerDialog::OnClassPickerCanceled)
		]
	];

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SBox)
			.WidthOverride(610.0f)
			[
				Container
			]
		]
	];

	if (WeakParentWindow.IsValid())
	{
		if (bExpandCustomClassPicker && ClassViewer)
		{
			WeakParentWindow.Pin().Get()->SetWidgetToFocusOnActivate(ClassViewer);
		}
	}
}

bool SBangoScriptPickerDialog::PickClass(const FText& TitleText, const FClassViewerInitializationOptions& ClassViewerOptions, UClass*& OutChosenClass, UClass* AssetType)
{
	// Create the window to pick the class
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(TitleText)
		.SizingRule( ESizingRule::Autosized )
		.ClientSize( FVector2D( 0.f, 300.f ))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SBangoScriptPickerDialog> ClassPickerDialog = SNew(SBangoScriptPickerDialog)
		.ParentWindow(PickerWindow)
		.Options(ClassViewerOptions)
		.AssetType(AssetType);

	PickerWindow->SetContent(ClassPickerDialog);

	GEditor->EditorAddModalWindow(PickerWindow);

	if (ClassPickerDialog->bPressedOk)
	{
		OutChosenClass = ClassPickerDialog->ChosenClass;
		return true;
	}
	else
	{
		// Ok was not selected, NULL the class
		OutChosenClass = NULL;
		return false;
	}
}


void SBangoScriptPickerDialog::OnClassPicked(UClass* InChosenClass)
{
	ChosenClass = InChosenClass;
}

FReply SBangoScriptPickerDialog::OnDefaultClassPicked(UClass* InChosenClass)
{
	ChosenClass = InChosenClass;
	bPressedOk = true;
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SBangoScriptPickerDialog::OnClassPickerConfirmed()
{
	if (!bAllowNone && ChosenClass == NULL)
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("EditorFactories", "MustChooseClassWarning", "You must choose a class."));
	}
	else
	{
		bPressedOk = true;

		if (WeakParentWindow.IsValid())
		{
			WeakParentWindow.Pin()->RequestDestroyWindow();
		}
	}
	return FReply::Handled();
}

FReply SBangoScriptPickerDialog::OnClassPickerCanceled()
{
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SBangoScriptPickerDialog::OnDefaultAreaExpansionChanged(bool bExpanded)
{
	if (bExpanded && WeakParentWindow.IsValid() && ClassViewer)
	{
		WeakParentWindow.Pin().Get()->SetWidgetToFocusOnActivate(ClassViewer);
	}
}

void SBangoScriptPickerDialog::OnCustomAreaExpansionChanged(bool bExpanded)
{
	check(ClassViewer);
	if (bExpanded && WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin().Get()->SetWidgetToFocusOnActivate(ClassViewer);
	}
}

EVisibility SBangoScriptPickerDialog::GetSelectButtonVisibility() const
{
	EVisibility ButtonVisibility = EVisibility::Hidden;
	if(bAllowNone || ChosenClass != nullptr )
	{
		ButtonVisibility = EVisibility::Visible;
	}
	return ButtonVisibility;
}

/** Overridden from SWidget: Called when a key is pressed down - capturing copy */
FReply SBangoScriptPickerDialog::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	if (ClassViewer)
	{
		WeakParentWindow.Pin().Get()->SetWidgetToFocusOnActivate(ClassViewer);
	}

	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnClassPickerCanceled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		OnClassPickerConfirmed();
	}
	else if (ClassViewer)
	{
		return ClassViewer->OnKeyDown(MyGeometry, InKeyEvent);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
