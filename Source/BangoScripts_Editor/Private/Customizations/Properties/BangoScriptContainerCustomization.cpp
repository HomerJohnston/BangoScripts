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

#include "Private/BangoEditorStyle.h"
#include "Private/Subsystems/BangoLevelScriptsEditorSubsystem.h"
#include "Private/BlueprintEditor/BangoScriptBlueprintEditor.h"
#include "Private/Widgets/SBangoGraphEditor.h"
#include "Utilities/BangoEditorUtility.h"
#include "Widgets/SBangoScriptClassViewer.h"
#include "Widgets/SBangoScriptPropertyEditorClass.h"
#include "Widgets/Colors/SColorBlock.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

class FWidgetBlueprintEditor;

// ================================================================================================

FBangoScriptContainerCustomization::FBangoScriptContainerCustomization()
{
	PostScriptCreated.AddRaw(this, &FBangoScriptContainerCustomization::OnPostScriptCreatedOrRenamed);
	PreScriptDeleted.AddRaw(this, &FBangoScriptContainerCustomization::OnPreScriptUnsetOrDeleted);
	
	FBangoEditorDelegates::OnScriptGenerated.AddRaw(this, &FBangoScriptContainerCustomization::OnPostScriptCreatedOrRenamed);
	FEditorDelegates::OnMapLoad.AddRaw(this, &FBangoScriptContainerCustomization::OnMapLoad);
}

// ----------------------------------------------

FBangoScriptContainerCustomization::~FBangoScriptContainerCustomization()
{
	FBangoEditorDelegates::OnScriptGenerated.RemoveAll(this);
	FEditorDelegates::OnMapLoad.RemoveAll(this);
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
	
	// Early Setup
	ScriptContainerProperty = PropertyHandle;
	ScriptClassProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBangoScriptContainer, ScriptClass));
	CurrentGraph = GetPrimaryEventGraph();
	
	// Header Row Setup
	HeaderRow.NameContent()
	.HAlign(HAlign_Fill)
	[
		SAssignNew(HeaderNameContent, SHorizontalBox)
	];
	
	HeaderRow.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SAssignNew(HeaderValueContent, SHorizontalBox)
	];
	
	HeaderRow.ResetToDefaultContent()
	[
		SNullWidget::NullWidget	
	];
	
	UpdateHeaderRowNameAndValueContent();
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (PropertyHandle->GetNumOuterObjects() != 1)
	{
		return;
	}
	
	auto SharedVisibilityAttribute = TAttribute<EVisibility>::CreateSP(this, &FBangoScriptContainerCustomization::Visibility_HasValidGraph);
	
	// Description field
	TSharedPtr<IPropertyHandle> DescriptionProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBangoScriptContainer, Description));
	IDetailPropertyRow& DescriptionPropertyRow = ChildBuilder.AddProperty(DescriptionProperty.ToSharedRef());
	
	TSharedPtr<SWidget> DescriptionNameWidget, DescriptionValueWidget;
	DescriptionPropertyRow.GetDefaultWidgets(DescriptionNameWidget, DescriptionValueWidget);
	DescriptionPropertyRow.Visibility(SharedVisibilityAttribute);
	DescriptionPropertyRow.CustomWidget(true)
	.NameContent()
	[
		DescriptionNameWidget.ToSharedRef()
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		DescriptionValueWidget.ToSharedRef()
	];
	
	// Embedded Graph Widget
	FDetailWidgetRow& GraphRow = ChildBuilder.AddCustomRow(LOCTEXT("BangoScriptHolder_SearchTerm", "Bango"))
	.ShouldAutoExpand(true)
	[
		SAssignNew(GraphWidgetBox, SVerticalBox)
	];
	
	GraphRow.Visibility(SharedVisibilityAttribute);

	UpdateGraphWidgetBox();
	
	// Script Inputs
	TSharedPtr<IPropertyHandle> ScriptInputsProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBangoScriptContainer, ScriptInputs));
	IDetailPropertyRow& ScriptsInputRow = ChildBuilder.AddProperty(ScriptInputsProperty.ToSharedRef());
	
	auto ScriptInputsVisibilityAttribute = TAttribute<EVisibility>::CreateSP(this, &FBangoScriptContainerCustomization::Visibility_HasScriptInputs);
	ScriptsInputRow.Visibility(ScriptInputsVisibilityAttribute);

	TSharedPtr<SWidget> ScriptInputsNameWidget, ScriptInputsValueWidget;
	ScriptsInputRow.GetDefaultWidgets(ScriptInputsNameWidget, ScriptInputsValueWidget);
				
	ScriptsInputRow.CustomWidget(true)
	.NameContent()
	[
		ScriptInputsNameWidget.ToSharedRef()
	]
	.ValueContent()
	[
		SNew(SBox)
		.MinDesiredHeight(22)
		[
			SNew(SButton)
			.Text(this, &FBangoScriptContainerCustomization::Text_RefreshScriptInputs)
			.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
			.NormalPaddingOverride(FMargin(8, 0, 8, 0))
			.PressedPaddingOverride(FMargin(8, 0.5, 8, -0.5))
			.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_RefreshScriptInputs)
			//.ButtonColorAndOpacity(this, &FBangoScriptContainerCustomization::ButtonColorAndOpacity_RefreshScriptInputs)
			.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_RefreshScriptInputs)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)	
		]
	];
}

// ----------------------------------------------

EVisibility FBangoScriptContainerCustomization::Visibility_HasValidGraph() const
{
	return CurrentGraph.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
}

// ----------------------------------------------

EVisibility FBangoScriptContainerCustomization::Visibility_HasNoValidGraph() const
{
	return CurrentGraph.IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
}

// ----------------------------------------------

EVisibility FBangoScriptContainerCustomization::Visibility_HasScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	if (!Outer || !ScriptContainer)
	{
		return EVisibility::Collapsed;
	}
	
	if (ScriptContainer->ScriptInputs.GetNumPropertiesInBag() > 0)
	{
		return EVisibility::Visible;
	}
	
	if (ScriptContainer->AreScriptInputsOutDated())
	{
		return EVisibility::Visible;
	}
	
	return EVisibility::Collapsed;
}

// ----------------------------------------------

EVisibility FBangoScriptContainerCustomization::Visibility_DeleteUnsetButton() const
{
	return HasScriptClassAssigned() ? EVisibility::Visible : EVisibility::Collapsed;
}

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_CreateScript()
{
	TArray<UPackage*> Packages;
	
	void* ScriptContainerPtr = nullptr;
	ScriptContainerProperty->GetValueData(ScriptContainerPtr);
	
	IBangoScriptHolderInterface& ScriptHolder = GetScriptHolder();
	FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();
	ScriptContainer.bNewLeveScriptRequested = true;
	FString BlueprintName = ScriptHolder._getUObject()->GetFName().ToString();
	
	FBangoEditorDelegates::OnScriptContainerCreated.Broadcast(GetScriptHolder(), BlueprintName);
	
	ScriptContainerProperty->SetExpanded(true);
		
	return FReply::Handled();
}

// ----------------------------------------------

FText FBangoScriptContainerCustomization::Text_UnsetDeleteScript() const
{
	switch (GetScriptType())
	{
		case EBangoScriptType::None:
		{
			if (HasScriptClassAssigned())
			{
				return LOCTEXT("ScriptContainerCustomization_UnsetLevelScript", "Clear");
			}
			else
			{
				return INVTEXT("---");
			}
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
	return INVTEXT("---");
}

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_UnsetDeleteScript()
{
	switch (GetScriptType())
	{
		case EBangoScriptType::None:
		{
			IBangoScriptHolderInterface& ScriptHolder = GetScriptHolder();
			FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();

			UObject* Outer = ScriptHolder._getUObject();
			Outer->Modify();

			ScriptContainer.Unset();
			
			SendDummyPECPEvent(Outer);
			break;
		}
		case EBangoScriptType::LevelScript:
		{
			EAppReturnType::Type Reply = FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteLevelScriptConfirmation_PromptMessage", "This will delete the level script graph asset. Are you sure?"));

			if (Reply == EAppReturnType::Ok)
			{
				PreScriptDeleted.Broadcast();
			
				IBangoScriptHolderInterface& ScriptHolder = GetScriptHolder();
				FBangoScriptContainer& ScriptContainer = ScriptHolder.GetScriptContainer();

				UObject* Outer = ScriptHolder._getUObject();
				Outer->Modify();

				ScriptContainer.MarkDeleted();
				
				FBangoEditorDelegates::OnScriptContainerDestroyed.Broadcast(ScriptHolder);
				
				ScriptContainer.Unset();
				
				SendDummyPECPEvent(Outer);
			}

			break;
		}
		case EBangoScriptType::ContentAssetScript:
		{
			UObject* Outer;
			FBangoScriptContainer* ScriptContainer;
			GetScriptContainerAndOuter(Outer, ScriptContainer);
				
			if (ScriptContainer->ScriptInputs.GetNumPropertiesInBag() > 0)
			{
				EAppReturnType::Type Reply = FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteLevelScriptConfirmation_PromptMessage", "This will clear any set script input values. Are you sure?"));

				if (Reply == EAppReturnType::Ok)
				{
					Outer->Modify();
					ScriptContainer->Unset();
				}
			}
			else
			{
				Outer->Modify();
				ScriptContainer->Unset();
			}	
			
			SendDummyPECPEvent(Outer);
			
			break;
		}
	}
		
	UpdateGraphWidgetBox();
	UpdateHeaderRowNameAndValueContent();
	
	return FReply::Handled();
}

// ----------------------------------------------

bool FBangoScriptContainerCustomization::IsEnabled_DeleteUnsetButton() const
{
	if (GetScriptClass())
	{
		// Will display "Delete" or "Unset" depending on the type
		return true;
	}
	
	if (IsScriptClassStale())
	{
		// Will display "Clear"
		return true;
	}
	
	return false;
}

// ----------------------------------------------

bool FBangoScriptContainerCustomization::IsEnabled_ScriptClassPicker() const
{
	switch (GetScriptType())
	{
		case EBangoScriptType::None:
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

// ----------------------------------------------

bool FBangoScriptContainerCustomization::IsEnabled_CreateLevelScriptButton() const
{
	// If there is already a script assigned, we can't create a level script
	if (GetScriptClass())
	{
		return false;
	}
	
	if (IsScriptClassStale())
	{
		return false;
	}
	
	return true;
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::OnSetClass_ScriptClass(const UClass* Class) const
{
	ScriptClassProperty->SetValue(Class);
}

// ----------------------------------------------

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
	TSharedRef<FBangoScriptBlueprintEditor> NewBlueprintEditor(new FBangoScriptBlueprintEditor());

	const bool bShouldOpenInDefaultsMode = false;
	TArray<UBlueprint*> Blueprints;

	UBlueprint* Blueprint = GetBlueprint();
	
	if (Blueprint)
	{
		Blueprints.Add(Blueprint);
		NewBlueprintEditor->InitBlueprintEditor(EToolkitMode::Standalone, nullptr, Blueprints, bShouldOpenInDefaultsMode);
	}
	
	return FReply::Handled();

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

// ----------------------------------------------

FText FBangoScriptContainerCustomization::Text_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	if (ScriptContainer->AreScriptInputsOutDated())
	{
		return LOCTEXT("ScriptContainerCustomization_RefreshScriptInputs", "Needs Refresh");
	}
	
	return LOCTEXT("ScriptContainerCustomization_RefreshScriptInputs_UpToDate", "Up-to-date");
}

// ----------------------------------------------

bool FBangoScriptContainerCustomization::IsEnabled_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	return ScriptContainer->AreScriptInputsOutDated();
}

// ----------------------------------------------

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

// ----------------------------------------------

FReply FBangoScriptContainerCustomization::OnClicked_RefreshScriptInputs() const
{
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	Outer->Modify();
	ScriptContainer->UpdateScriptInputs();
	
	return FReply::Handled();
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::OnTextChanged_ScriptNameEditableText(const FText& Text)
{
	SetProposedScriptName(Text);
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::SetProposedScriptName(const FText& Text)
{
	ScriptNameText = Text;
	ProposedNameStatus = GetProposedNameStatus();
}

// ----------------------------------------------

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

// ----------------------------------------------

FSlateColor FBangoScriptContainerCustomization::ForegroundColor_ScriptNameEditableText() const
{
	return FocusedForegroundColor_ScriptNameEditableText().GetSpecifiedColor().Desaturate(0.25f);
}

// ----------------------------------------------

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

// ----------------------------------------------

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
	TSharedRef<FBangoScriptBlueprintEditor> BlueprintEditor = MakeShared<FBangoScriptBlueprintEditor>();
	
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
					.DesiredSizeOverride(FVector2D(32, 32))
					.Image(FBangoEditorStyle::GetImageBrush(BangoEditorBrushes.Icon_EditScript))
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

// ----------------------------------------------

void FBangoScriptContainerCustomization::OnPostScriptCreatedOrRenamed()
{
	UpdateGraphWidgetBox();	
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::OnPreScriptUnsetOrDeleted()
{
	UpdateGraphWidgetBox();
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::UpdateHeaderRowNameAndValueContent()
{
	HeaderNameContent->ClearChildren();
	HeaderValueContent->ClearChildren();
	
	HeaderNameContent->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ScripContainer_ScriptClassPropertyLabel", "Script"))
		.TextStyle(FAppStyle::Get(), "SmallText")	
	];
	
	if (IsScriptClassStale())
	{
		HeaderNameContent->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(12, 0, 2, 0)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock)
				.Color(Bango::Colors::OrangeRed)
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				.Padding(4, 0, 4, 0)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ScripContainer_ScriptClassMissing", "MISSING"))
					.ToolTipText(FText::Format(INVTEXT("{0}\n{1}"), FText::FromString(GetScriptClassPath()), LOCTEXT("MissingScriptClass_ToolTipText", "This can happen if you create a level script and do not save it.\nRestore from version control, or press the [Clear] button.") ))
					.ColorAndOpacity(Bango::Colors::White)
					.TextStyle(FAppStyle::Get(), "SmallText")	
				]
			]
		];
	}
	
	int32 Count = 0;
	
	auto AddColumnTo = [this, &Count] (TSharedPtr<SHorizontalBox> Box, TSharedPtr<SWidget> Widget, float WidthOverride = 0.0f)
	{
		if (!Widget.IsValid())
		{
			return;
		}
		
		HeaderValueContent->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Fill)
		.Padding(Count == 0 ? 0 : 3, 0, 0, 0)
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			.MinDesiredWidth(WidthOverride > 0.0f ? TOptional<float>(WidthOverride) : NullOpt)
			[
				Widget.ToSharedRef()
			]
		];
		
		Count++;
	};
	
	TSharedPtr<SWidget> ScriptClassDropdown = nullptr;
	TSharedPtr<SWidget> CreateLevelScriptButton = nullptr;
	TSharedPtr<SWidget> DeleteOrUnsetButton = nullptr;
	
	if (GetScriptType() != EBangoScriptType::LevelScript)
	{
		ScriptClassDropdown = SNew(SBangoScriptPropertyEditorClass)
		.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_CreateLevelScriptButton)
		.MetaClass(UBangoScript::StaticClass())
		.SelectedClass(this, &FBangoScriptContainerCustomization::SelectedClass_ScriptClass)
		.OnSetClass(this, &FBangoScriptContainerCustomization::OnSetClass_ScriptClass)
		.ToolTipText(LOCTEXT("ScriptClassSelector_ToolTipText", "Select a content script."));
	}
	else
	{
		ScriptClassDropdown = SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ScriptClassSelector_LevelScriptTextPlaceholder", "Level Script"))
				.TextStyle(FAppStyle::Get(), "SmallText")
			];
	}
	
	if (!HasScriptClassAssigned())
	{
		CreateLevelScriptButton = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_CreateLevelScriptButton)
		.Text(LOCTEXT("ScriptContainerCustomization_LevelScriptButtonLabel", "Level Script"))
		.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
		// .ToolTipText(LOCTEXT("CreateLevelScriptButton_ToolTipText", "Create a new local level script."))
		.ButtonColorAndOpacity(Bango::Colors::White)
		.ForegroundColor(Bango::Colors::White)
		.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_CreateScript)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center);
	}
	
	DeleteOrUnsetButton = SNew(SButton)
	.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
	.IsEnabled(this, &FBangoScriptContainerCustomization::IsEnabled_DeleteUnsetButton)
	.Visibility(this, &FBangoScriptContainerCustomization::Visibility_DeleteUnsetButton)
	.NormalPaddingOverride(FMargin(8, 0, 8, 0))
	.PressedPaddingOverride(FMargin(8, 0.5, 8, -0.5))
	// .ButtonColorAndOpacity(FLinearColor::Gray)
	.Text(Text_UnsetDeleteScript())
	.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
	.ForegroundColor(Bango::Colors::White)
	.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_UnsetDeleteScript)
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center);
	
	AddColumnTo(HeaderValueContent, ScriptClassDropdown, 120.0f);
	AddColumnTo(HeaderValueContent, CreateLevelScriptButton);
	AddColumnTo(HeaderValueContent, DeleteOrUnsetButton);
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::UpdateGraphWidgetBox()
{
	SGraphEditor::FGraphEditorEvents Events;
	
	GraphWidgetBox->ClearChildren();
	
	CurrentGraph = GetPrimaryEventGraph();
	
	if (!CurrentGraph.IsValid())
	{
		return;
	}
	
	UObject* Outer;
	FBangoScriptContainer* ScriptContainer;
	GetScriptContainerAndOuter(Outer, ScriptContainer);
	
	if (!Outer)
	{
		return;
	}
		
	TSharedRef<SGraphEditor> GraphEditor = SNew(SGraphEditor)
	.GraphToEdit(CurrentGraph.Get())
	.IsEditable(false)
	.GraphEvents(Events)
	.ShowGraphStateOverlay(false);
	
	TSharedPtr<SOverlay> GraphOverlayWidget;
	
	GraphWidgetBox->AddSlot()
	.AutoHeight()
	[
		SAssignNew(GraphOverlayWidget, SOverlay)
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
		.Padding(4-22, 4, 0, 0)
		[
			SNew(SButton)
			.ContentPadding(4)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.OnClicked(this, &FBangoScriptContainerCustomization::OnClicked_EditScript)
			[
				SNew(SImage)
				.Image(FBangoEditorStyle::GetImageBrush(BangoEditorBrushes.Icon_EditScript))
				.DesiredSizeOverride(FVector2D(20.0f, 20.0f))
			]
		]
	];
	
	if (GetScriptType() == EBangoScriptType::LevelScript)
	{
		GraphOverlayWidget->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(8-22, 0, 0, 8)
		[
			SNew(STextBlock)
			.Text(FText::Format
				(
					//INVTEXT("{0}\n{1}\n{2}{3}"),
					INVTEXT("{0}\n{1}{2}"),
					FText::FromName( GetBlueprint()->GetFName() ),
					// FText::FromString( ScriptContainer->GetGuid().ToString() ),
					FText::FromString( FPackageName::GetShortName( GetBlueprint()->GetPackage()->GetPathName() ) ),
					FText::FromString( GetBlueprint()->GetPackage()->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension())
				))
			.Font(FCoreStyle::GetDefaultFontStyle("Normal", 8))
			.ColorAndOpacity(Bango::Colors::Gray)
		];
	}
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

// ----------------------------------------------

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

bool FBangoScriptContainerCustomization::HasScriptClassAssigned() const
{
	FString StringValue;

	if (ScriptClassProperty->GetValueAsFormattedString(StringValue))
	{
		if (StringValue.IsEmpty() || StringValue == FName(NAME_None).ToString())
		{
			return false;
		}
	}

	return true;
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

bool FBangoScriptContainerCustomization::IsScriptClassStale() const
{
	FString StringValue;

	if (ScriptClassProperty->GetValueAsFormattedString(StringValue))
	{
		if (StringValue.IsEmpty() || StringValue == FName(NAME_None).ToString())
		{
			return false;
		}
		
		FSoftClassPath ClassPath(StringValue);
		FTopLevelAssetPath AssetPath = ClassPath.GetAssetPath();

		if (FPackageName::DoesPackageExist(AssetPath.GetPackageName().ToString()) || FindObject<UObject>(nullptr, *StringValue))
		{
			return false;
		}
	}

	return true;
}

FString FBangoScriptContainerCustomization::GetScriptClassPath() const
{
	FString StringValue;

	if (ScriptClassProperty->GetValueAsFormattedString(StringValue))
	{
		if (StringValue.IsEmpty())
		{
			return "INVALID";
		}
		
		FSoftClassPath ClassPath(StringValue);
		FTopLevelAssetPath AssetPath = ClassPath.GetAssetPath();
		
		return AssetPath.GetPackageName().ToString();
	}

	return "INVALID";
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap)
{
	// If we don't get rid of the blueprint graph, we get an editor crash when switching maps.
	GraphWidgetBox->ClearChildren();
	CurrentGraph = nullptr;
}

// ----------------------------------------------

void FBangoScriptContainerCustomization::GetScriptContainerAndOuter(UObject*& Outer, FBangoScriptContainer*& ScriptContainer) const
{
	// TODO URGENT I am sometimes getting a crash here when I delete a script or an Outer??
	Outer = GetOuter();
	
	if (!Outer)
	{
		return;
	}
	
	void* ScriptContainerPtr = nullptr;
	
	FPropertyAccess::Result Result = ScriptContainerProperty->GetValueData(ScriptContainerPtr);
	
	if (Result != FPropertyAccess::Result::Success)
	{
		checkNoEntry();
	}
	
	ScriptContainer = reinterpret_cast<FBangoScriptContainer*>(ScriptContainerPtr);
}

// ----------------------------------------------

IBangoScriptHolderInterface& FBangoScriptContainerCustomization::GetScriptHolder() const
{
	IBangoScriptHolderInterface* ScriptHolder = Cast<IBangoScriptHolderInterface>(GetOuter());
	check(ScriptHolder);
	
	return *ScriptHolder;
}

EBangoScriptStatus FBangoScriptContainerCustomization::GetScriptStatus() const
{
	return EBangoScriptStatus::None;
}

// ----------------------------------------------

EBangoScriptType FBangoScriptContainerCustomization::GetScriptType() const
{
	TSubclassOf<UBangoScript> ScriptClass = GetScriptClass();

	if (!ScriptClass)
	{
		return EBangoScriptType::None;
	}
	
	if (ScriptClass->GetName().StartsWith(Bango::Editor::GetLevelScriptNamePrefix()))
	{
		return EBangoScriptType::LevelScript;
	}
	
	return EBangoScriptType::ContentAssetScript;
}

void FBangoScriptContainerCustomization::SendDummyPECPEvent(UObject* Object) const
{
	if (Object == nullptr)
	{
		IBangoScriptHolderInterface& ScriptHolder = GetScriptHolder();
		Object = ScriptHolder._getUObject();	
	}
	
	// Throw a dummy property change event. UBangoScriptComponent will use this to update its billboard.
	FPropertyChangedEvent NewScriptDummyChangedEvent(nullptr, EPropertyChangeType::Unspecified, {});
	Object->PostEditChangeProperty(NewScriptDummyChangedEvent);
}

// ----------------------------------------------

#undef LOCTEXT_NAMESPACE