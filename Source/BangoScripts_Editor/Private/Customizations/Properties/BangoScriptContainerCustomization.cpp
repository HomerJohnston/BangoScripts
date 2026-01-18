#include "BangoScripts/Editor/Customizations/Properties/BangoScriptContainerCustomization.h"

#include "BlueprintEditor.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "SEditorViewport.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "PropertyEditorModule.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "ClassViewerFilter/BangoClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"

#include "Private/BangoEditorStyle.h"
#include "Private/Subsystems/BangoLevelScriptsEditorSubsystem.h"
#include "Private/BlueprintEditor/BangoBlueprintEditor.h"
#include "Private/Widgets/SBangoGraphEditor.h"
#include "Utilities/BangoEditorUtility.h"
#include "Widgets/SBangoScriptClassViewer.h"
#include "Widgets/SBangoScriptPickerDialog.h"
#include "Widgets/SBangoScriptPropertyEditorClass.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

class FWidgetBlueprintEditor;

// ==============================================

FBangoScriptContainerCustomization::FBangoScriptContainerCustomization()
{
	PostScriptCreated.AddRaw(this, &FBangoScriptContainerCustomization::OnPostScriptCreatedOrRenamed);
	PreScriptDeleted.AddRaw(this, &FBangoScriptContainerCustomization::OnPreScriptDeleted);
	
	UBangoLevelScriptsEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UBangoLevelScriptsEditorSubsystem>();
	
	if (Subsystem)
	{
		Subsystem->OnScriptGenerated.AddRaw(this, &FBangoScriptContainerCustomization::OnPostScriptCreatedOrRenamed);
	}
	
	//BangoScriptClassViewerFilters.Add( MakeShared<FBangoScriptsScriptContainerClassViewerFilter>() );
}

// ----------------------------------------------

FBangoScriptContainerCustomization::~FBangoScriptContainerCustomization()
{
	UBangoLevelScriptsEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UBangoLevelScriptsEditorSubsystem>();
	
	if (Subsystem)
	{
		Subsystem->OnScriptGenerated.RemoveAll(this);
	}
	
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
}

// ----------------------------------------------

TSharedRef<IPropertyTypeCustomization> FBangoScriptContainerCustomization::MakeInstance()
{
	return MakeShared<FBangoScriptContainerCustomization>();
}

// ----------------------------------------------

UEdGraph* FBangoScriptContainerCustomization::GetPrimaryEventGraph() const
{
	UBlueprint* Blueprint = GetBlueprint();
	
	if (!Blueprint)
	{
		return nullptr;
	}
	
	if (Blueprint->UbergraphPages.Num() > 0)
	{
		return Blueprint->UbergraphPages[0];
	}

	return nullptr;
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Validity for customization display
	if (PropertyHandle->GetNumOuterObjects() != 1)
	{
		HeaderRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MultipleScriptsEdited_Warning", "    <Multiple Scripts>"))
			.TextStyle(FAppStyle::Get(), "SmallText")
		];
		
		return;
	}
	
	// PropertyHandle->SetExpanded(true);
	
	// Setup
	ScriptContainerProperty = PropertyHandle;
	ScriptClassProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBangoScriptContainer, ScriptClass));
		
	CurrentGraph = GetPrimaryEventGraph();
		
	HeaderRow.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ScripContainer_ScriptClassPropertyLabel", "Script"))
		.TextStyle(FAppStyle::Get(), "SmallText")
	];
	
	HeaderRow.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex(this, &FBangoScriptContainerCustomization::WidgetIndex_GraphEditor)
		+ SWidgetSwitcher::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				
				SNew(SButton)
				.Text(INVTEXT("Pick Script"))
				.OnClicked_Lambda([]()
				{
					FClassViewerInitializationOptions Options;
					//Options.ClassFilters.Add(MakeShared<FNewNodeClassFilter>(UDataAsset::StaticClass()));
					UClass* ScriptClass = nullptr;
					
					SBangoScriptPickerDialog::PickClass(INVTEXT("Test"), Options, ScriptClass, UBangoScript::StaticClass());
					
					return FReply::Handled();
				})
				
				/*
				SNew(SClassPropertyEntryBox)
				.MetaClass(UBangoScript::StaticClass())
				.AllowNone(true)
				//.ClassViewerFilters(BangoScriptClassViewerFilters)
				.AllowAbstract(false)
				.OnSetClass(this, &FBangoScriptContainerCustomization::OnSetClass_ScriptClass)
				.SelectedClass(this, &FBangoScriptContainerCustomization::SelectedClass_ScriptClass)
				.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_ScriptClassPicker)
				*/
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("ScriptContainerCustomization_CreateLevelScriptButtonLabel", "Create New Level Script"))
				.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
				.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_CreateScript)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_CreateLevelScriptButton)
			]
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 6, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("ScriptContainerCustomization_EditScriptButtonLabel", "Edit"))
				.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
				.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_EditScript)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
				.NormalPaddingOverride(0)
				.PressedPaddingOverride(0)
				//.ButtonColorAndOpacity(FLinearColor::Red)
				.Text(this, &FBangoScriptContainerCustomization::Text_UnsetDeleteScript)
				.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
				.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_UnsetDeleteScript)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
			]
		]
	];
	
	HeaderRow.ResetToDefaultContent()
	[
		SNullWidget::NullWidget	
	];
	
	UBlueprint* Blueprint = GetBlueprint();
	
	if (Blueprint)
	{
		SetProposedScriptName(FText::FromString(Blueprint->GetName()));
	}
	
	// We kill the graph widgets immediately to avoid garbage collection and file access issues
	FEditorDelegates::OnMapLoad.AddSP(this, &FBangoScriptContainerCustomization::OnMapLoad);
	FBangoEditorDelegates::OnScriptContainerDestroyed.AddSP(this, &FBangoScriptContainerCustomization::OnScriptContainerDestroyed);
}

// ----------------------------------------------

int FBangoScriptContainerCustomization::WidgetIndex_CreateDeleteScriptButtons() const
{
	if (IsValid(GetScriptClass()))
	{
		return 1;
	}
	
	return 0;
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (PropertyHandle->GetNumOuterObjects() != 1)
	{
		return;
	}
	
	Box = SNew(SVerticalBox);
	
	ChildBuilder.AddCustomRow(LOCTEXT("BangoScriptHolder_SearchTerm", "Bango"))
	.ShouldAutoExpand(true)
	[
		Box.ToSharedRef()
	];

	UpdateBox();
	
	// Add native children (Description field etc.)
	
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);
	
	for (uint32 ChildIndex = 0; ChildIndex <  NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex);
		
		if (ChildHandle->IsValidHandle() && ChildHandle->IsEditable())
		{
			if (ChildHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(FBangoScriptContainer, ScriptInputs))
			{
				IDetailPropertyRow& Row = ChildBuilder.AddProperty(ChildHandle.ToSharedRef());

				TSharedPtr<SWidget> NameWidget, ValueWidget;
				Row.GetDefaultWidgets(NameWidget, ValueWidget);
				
				Row.CustomWidget(true)
				.NameContent()
				[
					NameWidget.ToSharedRef()
				]
				.ValueContent()
				[
					SNew(SButton)
					.Text(this, &FBangoScriptContainerCustomization::Text_RefreshScriptInputs)
					.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
					.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_RefreshScriptInputs)
					.ButtonColorAndOpacity(this, &FBangoScriptContainerCustomization::ButtonColorAndOpacity_RefreshScriptInputs)
					.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_RefreshScriptInputs)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
				];
			}
			else
			{
				ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}
}

// ----------------------------------------------

int FBangoScriptContainerCustomization::WidgetIndex_GraphEditor() const
{
	return GetScriptClass() ? 1 : 0;
}

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_CreateScript()
{
	TArray<UPackage*> Packages;
	
	void* ScriptContainerPtr = nullptr;
	ScriptContainerProperty->GetValueData(ScriptContainerPtr);
	
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(Outer, ScriptContainer, *Outer->GetFName().ToString());
	
	ScriptContainerProperty->SetExpanded(true);
	/*
	ScriptClassProperty->GetOuterPackages(Packages);
	
	if (Packages.Num() != 1)
	{
		return FReply::Handled();
	}

	void* GuidPtr;
	GuidProperty->GetValueData(GuidPtr);
	FGuid* Guid = reinterpret_cast<FGuid*>(GuidPtr);
	
	FString BPName;
	UPackage* ScriptPackage = Bango::Editor::MakePackageForScript(ScriptClassProperty, Packages[0], BPName, *Guid);
	
	if (!ScriptPackage)
	{
		return FReply::Handled();
	}
	
	UBlueprint* Script = Bango::Editor::MakeScriptAsset(ScriptPackage, BPName, *Guid);
	
	if (!Script)
	{
		return FReply::Handled();
	}

	//if (Bango::Editor::SaveScriptPackage(ScriptPackage, Script))
	//{
		ScriptClassProperty->SetValue(Script->GeneratedClass);
		PostScriptCreated.Broadcast();
	//}
	*/
	
	return FReply::Handled();
}

FText FBangoScriptContainerCustomization::Text_UnsetDeleteScript() const
{
	switch (GetScriptType())
	{
		case EBangoScriptType::Unset:
		{
			return INVTEXT("-");
		}
		case EBangoScriptType::LevelScript:
		{			
			return LOCTEXT("ScriptContainerCustomization_DeleteLevelScript", "Delete");
		}
		case EBangoScriptType::ContentAssetScript:
		{
			return LOCTEXT("ScriptContainerCustomization_UnsetLevelScript", "Unset");
		}
	}
	
	checkNoEntry();
	return INVTEXT("-");
}

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_UnsetDeleteScript()
{
	switch (GetScriptType())
	{
		case EBangoScriptType::Unset:
		{
			return FReply::Handled();
		}
		case EBangoScriptType::LevelScript:
		{
			EAppReturnType::Type Reply = FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteLevelScriptConfirmation_PromptMessage", "This will delete the level script graph asset. Are you sure?"));

			if (Reply == EAppReturnType::Ok)
			{
				PreScriptDeleted.Broadcast();
				
				UObject* Outer;
				FBangoScriptContainer* ScriptContainer;
				GetScriptContainerAndOuter(Outer, ScriptContainer);
			
				Outer->Modify();
				FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(Outer, ScriptContainer);
				
				ScriptContainer->Unset();
			}

			return FReply::Handled();				
		}
		case EBangoScriptType::ContentAssetScript:
		{
			UObject* Outer;
			FBangoScriptContainer* ScriptContainer;
			GetScriptContainerAndOuter(Outer, ScriptContainer);
			
			Outer->Modify();
			ScriptContainer->Unset();
		}
	}
	
	return FReply::Handled();
}

bool FBangoScriptContainerCustomization::IsEnabled_ScriptClassPicker() const
{
	switch (GetScriptType())
	{
		case EBangoScriptType::Unset:
		{
			return true;
		}
		case EBangoScriptType::LevelScript:
		{
			return false;
		}
		case EBangoScriptType::ContentAssetScript:
		{
			return true;
		}
	}
	
	checkNoEntry();
	return true;
}

bool FBangoScriptContainerCustomization::IsEnabled_CreateLevelScriptButton() const
{
	// If there is already a script assigned, we can't create a level script
	return !GetScriptClass();
}

void FBangoScriptContainerCustomization::OnSetClass_ScriptClass(const UClass* Class) const
{
	ScriptClassProperty->SetValue(Class);
}

const UClass* FBangoScriptContainerCustomization::SelectedClass_ScriptClass() const
{
	return GetScriptClass();
}

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_EditScript() const
{
	UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	if (Subsystem)
	{
		UBlueprint* ScriptBlueprint = GetBlueprint();
		
		if (ScriptBlueprint)
		{
			Subsystem->OpenEditorForAsset(ScriptBlueprint);
		}
	}
	
	return FReply::Handled();
}

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_EnlargeGraphView() const
{
#if 1
	TSharedRef<FBangoBlueprintEditor> NewBlueprintEditor(new FBangoBlueprintEditor());

	const bool bShouldOpenInDefaultsMode = false;
	TArray<UBlueprint*> Blueprints;
	Blueprints.Add(GetBlueprint());

	NewBlueprintEditor->InitBlueprintEditor(EToolkitMode::Standalone, nullptr, Blueprints, bShouldOpenInDefaultsMode);
	//NewBlueprintEditor->InitWidgetBlueprintEditor(EToolkitMode::Standalone, nullptr, Blueprints, bShouldOpenInDefaultsMode);
	
	return FReply::Handled();
#endif 

#if 0
	FViewport* Viewport = GEditor->GetActiveViewport();
	FViewportClient* ViewportClient = Viewport->GetClient();
	FEditorViewportClient* EditorViewportClient = static_cast<FEditorViewportClient*>(ViewportClient);
	FDeprecateSlateVector2D VPWindowPos = EditorViewportClient->GetEditorViewportWidget().Get()->GetCachedGeometry().GetAbsolutePosition();

	TSharedPtr<SWindow> UEWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	
	FVector2D WindowPos(0);
	FVector2D WindowSize(400);
	
	if (UEWindow.IsValid())
	{
		FVector2D MainWindowSize = UEWindow->GetClientSizeInScreen();
		FVector2D MainWindowPos = UEWindow->GetPositionInScreen();

		float CornerInset = FMath::RoundToFloat( 0.1f * MainWindowSize.Y );

		WindowPos = FVector2D(MainWindowPos.X + CornerInset, MainWindowPos.Y + CornerInset);
		WindowSize = FVector2D(MainWindowSize.X - 2.0f * CornerInset, MainWindowSize.Y - 2.0f * CornerInset);
	}

	TSharedRef<SWidget> Popout = GetPopoutGraphEditor(WindowSize);
	
	FSlateApplication::Get().PushMenu(
		FSlateApplication::Get().GetUserFocusedWidget(0).ToSharedRef(),
		FWidgetPath(),
		Popout,
		FDeprecateSlateVector2D(WindowPos.X, WindowPos.Y),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		true);
	
	return FReply::Handled();
#endif
}

FReply FBangoScriptContainerCustomization::OnClicked_RenameScript() const
{
	return FReply::Handled();
}

FText FBangoScriptContainerCustomization::Text_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	if (ScriptContainer->AreScriptInputsOutDated())
	{
		return LOCTEXT("ScriptContainerCustomization_RefreshScriptInputs", "Refresh");
	}
	
	return LOCTEXT("ScriptContainerCustomization_RefreshScriptInputs_UpToDate", "Up-to-date");
}

bool FBangoScriptContainerCustomization::IsEnabled_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	return ScriptContainer->AreScriptInputsOutDated();
}

FSlateColor FBangoScriptContainerCustomization::ButtonColorAndOpacity_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	if (ScriptContainer->AreScriptInputsOutDated())
	{
		return Bango::Colors::OrangeRed;
	}
	
	return FSlateColor::UseStyle();
}

FReply FBangoScriptContainerCustomization::OnClicked_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	Outer->Modify();
	ScriptContainer->UpdateScriptInputs();
	
	return FReply::Handled();
}

void FBangoScriptContainerCustomization::OnTextChanged_ScriptNameEditableText(const FText& Text)
{
	SetProposedScriptName(Text);
}

void FBangoScriptContainerCustomization::SetProposedScriptName(const FText& Text)
{
	ScriptNameText = Text;
	ProposedNameStatus = GetProposedNameStatus();
}

EBangoScriptRenameStatus FBangoScriptContainerCustomization::GetProposedNameStatus()
{
	UPackage* Package = GetBlueprint()->GetPackage();
	
	if (Package)
	{
		TArray<UObject*> Objects;
		GetObjectsWithPackage(Package, Objects, false);
		
		FString ProposedBlueprintClassName = ScriptNameText.ToString();
		
		if (ProposedBlueprintClassName == GetBlueprint()->GetName())
		{
			return EBangoScriptRenameStatus::MatchesCurrent;
		}
		
		if (!ProposedBlueprintClassName.EndsWith(TEXT("_C")))
		{
			ProposedBlueprintClassName.Append(TEXT("_C"));
		}
		
		bool bMatch = false;
		
		for (UObject* Object : Objects)
		{
			// I need to make sure we don't cause *any* object name conflicts
			/*
			if (!Object->IsA<UBlueprintGeneratedClass>())
			{
				continue;
			}
			*/
			
			if (ProposedBlueprintClassName == Object->GetName())
			{
				bMatch = true;
				break;
			}
		}
				
		if (bMatch)
		{
			return EBangoScriptRenameStatus::MatchesOther;
		}

		// TODO check for valid chars
		return EBangoScriptRenameStatus::ValidNewName;
	}

	return EBangoScriptRenameStatus::InvalidNewName;
}

FSlateColor FBangoScriptContainerCustomization::ForegroundColor_ScriptNameEditableText() const
{
	return FocusedForegroundColor_ScriptNameEditableText().GetSpecifiedColor().Desaturate(0.25f);
}

FSlateColor FBangoScriptContainerCustomization::FocusedForegroundColor_ScriptNameEditableText() const
{
	switch (ProposedNameStatus)
	{
		case EBangoScriptRenameStatus::MatchesCurrent:
		{
			return Bango::Colors::Gray;
		}
		case EBangoScriptRenameStatus::MatchesOther:
		{
			return Bango::Colors::LightRed;
		}
		case EBangoScriptRenameStatus::ValidNewName:
		{
			return Bango::Colors::LightGreen;
		}
		default:
		{
			return Bango::Colors::Error;
		}
	}
}

bool FBangoScriptContainerCustomization::IsEnabled_RenameScriptButton() const
{
	switch (ProposedNameStatus)
	{
		case EBangoScriptRenameStatus::ValidNewName:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

// ----------------------------------------------

TSharedRef<SWidget> FBangoScriptContainerCustomization::GetPopoutGraphEditor(FVector2D WindowSize) const
{
	SGraphEditor::FGraphEditorEvents Events;
	
	const bool bShouldOpenInDefaultsMode = false;
	TSharedRef<FBangoBlueprintEditor> BlueprintEditor = MakeShared<FBangoBlueprintEditor>();
	
	// Not working. 
	BlueprintEditor->InitBlueprintEditor(EToolkitMode::Standalone, nullptr, { GetBlueprint() }, bShouldOpenInDefaultsMode);
	// BlueprintEditor->SetupGraphEditorEvents_Impl(GetBlueprint(), GetPrimaryEventGraph(), Events);
	
	return SNew(SBorder)
	.Padding(20)
	[
		SNew(SBox)
		.WidthOverride(WindowSize.X)
		.HeightOverride(WindowSize.Y)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBangoGraphEditor)
				.BlueprintEditor(BlueprintEditor)
				.GraphToEdit(GetPrimaryEventGraph())
				.IsEditable(false) // (true) // Not working
				.GraphEvents(Events)
				.ShowGraphStateOverlay(true)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(20)
			[
				SNew(SButton)
				.OnClicked_Lambda([this] ()
				{
					UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

					if (Subsystem)
					{
						UBlueprint* ScriptBlueprint = GetBlueprint();
						
						if (ScriptBlueprint)
						{
							Subsystem->OpenEditorForAsset(ScriptBlueprint);
						}
					}
	
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.DesiredSizeOverride(FVector2D(24, 24))
					.Image(FAppStyle::GetBrush("Sequencer.AllowAllEdits")) // TODO custom icon
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(20)
			[
				SNew(SButton)
				.OnClicked_Lambda([] ()
				{
					FSlateApplication::Get().ClearAllUserFocus();
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.DesiredSizeOverride(FVector2D(24, 24))
					.Image(FAppStyle::GetBrush("Icons.X"))
				]
			]
		]
	];
}

void FBangoScriptContainerCustomization::OnPostScriptCreatedOrRenamed()
{
	CurrentGraph = GetPrimaryEventGraph();
	
	UpdateBox();	
}

void FBangoScriptContainerCustomization::OnPreScriptDeleted()
{
	CurrentGraph = nullptr;
	
	UpdateBox();
}

void FBangoScriptContainerCustomization::UpdateBox()
{
	SGraphEditor::FGraphEditorEvents Events;
	
	Box->ClearChildren();
	
	if (!CurrentGraph.IsValid())
	{
		return;
	}
	
	TSharedRef<SGraphEditor> GraphEditor = SNew(SGraphEditor)
	.GraphToEdit(GetPrimaryEventGraph())
	.IsEditable(false)
	.GraphEvents(Events)
	.ShowGraphStateOverlay(false);
	
	Box->AddSlot()
	.AutoHeight()
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.HeightOverride(300)
			.Padding(-22, 0, 0, 0)
			[
				GraphEditor
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(8-22, 8, 0, 0)
		[
			SNew(SButton)
			.ContentPadding(0)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
			.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_EditScript)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Fullscreen"))
				.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(8-22, 0, 0, 8)
		[
			SNew(STextBlock)
			.Text(FText::Format
				(
					INVTEXT("{0}\n{1}{2}"),
					FText::FromName(GetBlueprint()->GetFName()),
					FText::FromString( FPackageName::GetShortName( GetBlueprint()->GetPackage()->GetPathName() ) ),
					FText::FromString( GetBlueprint()->GetPackage()->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension())
				))
			.Font(FCoreStyle::GetDefaultFontStyle("Normal", 8))
			.ColorAndOpacity(Bango::Colors::Gray)
		]
	];
}

// ----------------------------------------------

AActor* FBangoScriptContainerCustomization::GetOwnerActor() const
{
	TArray<UObject*> OuterObjects;
	ScriptClassProperty->GetOuterObjects(OuterObjects);
	
	if (OuterObjects.Num() > 0)
	{
		if (AActor* Actor = Cast<AActor>(OuterObjects[0]))
		{
			return Actor;
		}
		else if (UActorComponent* Component = Cast<UActorComponent>(OuterObjects[0]))
		{
			return Component->GetOwner();
		}
	}
	
	return nullptr;
}

UObject* FBangoScriptContainerCustomization::GetOuter() const
{
	TArray<UObject*> OuterObjects;
	ScriptClassProperty->GetOuterObjects(OuterObjects);
	
	if (OuterObjects.Num() == 1)
	{
		return OuterObjects[0];
	}
	
	return nullptr;
}

// ----------------------------------------------

UBlueprint* FBangoScriptContainerCustomization::GetBlueprint() const
{
	void* ScriptClassPtr = nullptr;
	
	if (ScriptClassProperty->GetValueData(ScriptClassPtr) == FPropertyAccess::Result::Success)
	{
		TSoftClassPtr<UBangoScript>* ScriptClass = reinterpret_cast<TSoftClassPtr<UBangoScript>*>(ScriptClassPtr);

		if (ScriptClass)
		{
			UObject* ScriptClassLoaded = ScriptClass->LoadSynchronous();
			
			TSubclassOf<UBangoScript> BangoScriptClassLoaded = Cast<UClass>(ScriptClassLoaded);
			
			if (UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(BangoScriptClassLoaded))
			{
				return Blueprint;
			}
		}
	}

	return nullptr;
}

// ----------------------------------------------

TSubclassOf<UBangoScript> FBangoScriptContainerCustomization::GetScriptClass() const
{
	UObject* ClassObject;
	
	if (ScriptClassProperty->GetValue(ClassObject) == FPropertyAccess::Success)
	{
		return Cast<UClass>(ClassObject);
	}

	return nullptr;
}

void FBangoScriptContainerCustomization::OnScriptContainerDestroyed(UObject* Object, FBangoScriptContainer* ScriptContainer)
{
	OnPreScriptDeleted();
}

void FBangoScriptContainerCustomization::OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap)
{
	// If we don't get rid of the blueprint graph, we get an editor crash when switching maps.
	Box->ClearChildren();
	CurrentGraph = nullptr;
}

void FBangoScriptContainerCustomization::GetScriptContainerAndOuter(UObject*& Outer, FBangoScriptContainer*& ScriptContainer) const
{
	Outer = GetOuter();
	check(Outer);
	
	void* ScriptContainerPtr = nullptr;
	
	FPropertyAccess::Result Result = ScriptContainerProperty->GetValueData(ScriptContainerPtr);
	
	if (Result != FPropertyAccess::Result::Success)
	{
		checkNoEntry();
	}
	
	ScriptContainer = reinterpret_cast<FBangoScriptContainer*>(ScriptContainerPtr);
}

EBangoScriptType FBangoScriptContainerCustomization::GetScriptType() const
{
	TSubclassOf<UBangoScript> ScriptClass = GetScriptClass();

	if (!ScriptClass)
	{
		return EBangoScriptType::Unset;
	}
	
	if (ScriptClass->GetName().StartsWith(Bango::Editor::GetLevelScriptNamePrefix()))
	{
		return EBangoScriptType::LevelScript;
	}
	
	return EBangoScriptType::ContentAssetScript;
}

// ----------------------------------------------

// ----------------------------------------------

#undef LOCTEXT_NAMESPACE