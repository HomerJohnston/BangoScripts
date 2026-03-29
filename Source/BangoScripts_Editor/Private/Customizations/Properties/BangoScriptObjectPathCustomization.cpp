#include "BangoScriptObjectPathCustomization.h"

#include "BangoScripts/Core/BangoScriptObjectPath.h"
#include "PropertyCustomizationHelpers.h"
#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"
#include "GameFramework/Actor.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

FBangoScriptObjectPathCustomization::FBangoScriptObjectPathCustomization()
{
}

FBangoScriptObjectPathCustomization::~FBangoScriptObjectPathCustomization()
{
}

TSharedRef<IPropertyTypeCustomization> FBangoScriptObjectPathCustomization::MakeInstance()
{
	return MakeShared<FBangoScriptObjectPathCustomization>();
}

void FBangoScriptObjectPathCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    ObjectPathStringProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBangoScriptObjectPath, Path));

    FOnGetAllowedClasses OnGetAllowedClasses = FOnGetAllowedClasses::CreateRaw(this, &FBangoScriptObjectPathCustomization::GetAllowedClasses);
    FOnShouldFilterActor OnShouldFilterActor = FOnShouldFilterActor::CreateRaw(this, &FBangoScriptObjectPathCustomization::ShouldFilterActor);
    FOnActorSelected OnActorSelectedFromPicker = FOnActorSelected::CreateRaw(this, &FBangoScriptObjectPathCustomization::ActorSelectedFromPicker);

    PathStringPropertyValueWidget = ObjectPathStringProperty->CreatePropertyValueWidgetWithCustomization(nullptr);
    PathStringPropertyValueWidget->SetEnabled(false);
    
    HeaderRow.ResetToDefaultContent()
    [
        SNullWidget::NullWidget
    ];
    
    HeaderRow.NameContent()
    [
        PropertyHandle->CreatePropertyNameWidget()
    ];

    HeaderRow.ValueContent()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .HAlign(HAlign_Fill)
        [
            PathStringPropertyValueWidget.ToSharedRef()
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SWidgetSwitcher)
            .WidgetIndex_Lambda( [this] () { return PathStringPropertyValueWidget->IsEnabled() ? 1 : 0; } )
            + SWidgetSwitcher::Slot()
            [
                SNew(SButton)
                .ToolTipText( LOCTEXT("UnlockObjectPathEditor", "Click to edit the object path.") )
                .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
                .OnClicked(FOnClicked::CreateRaw(this, &FBangoScriptObjectPathCustomization::OnClicked_UnlockButton))
                .ContentPadding(4.0f)
                .ForegroundColor(FSlateColor::UseForeground())
                .IsFocusable(false)
                [
                    SNew(SImage)
                    .Image(FAppStyle::GetBrush("Icons.Lock"))
                    .ColorAndOpacity(FSlateColor::UseForeground())
                ]
            ]
            + SWidgetSwitcher::Slot()
            [
                PropertyCustomizationHelpers::MakeInteractiveActorPicker(OnGetAllowedClasses, OnShouldFilterActor, OnActorSelectedFromPicker)
            ]
        ]
    ];
}

void FBangoScriptObjectPathCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FBangoScriptObjectPathCustomization::GetAllowedClasses(TArray<const UClass*>& AllowedClasses)
{
    AllowedClasses.Add(AActor::StaticClass());
}

bool FBangoScriptObjectPathCustomization::ShouldFilterActor(const AActor* Actor)
{
    TArray<UObject*> AllInners;
    constexpr bool bIncludeNestedObjects = true;
    
    GetObjectsWithOuter(Actor, AllInners, bIncludeNestedObjects);
    
    for (auto InnerIt = AllInners.CreateConstIterator(); InnerIt; ++InnerIt)
    {
        UObject* TestObject = *InnerIt;
        
        if (TestObject->Implements<UBangoScriptHolderInterface>())
        {
            return true;
        }
    }
    
    return false;
}

void FBangoScriptObjectPathCustomization::ActorSelectedFromPicker(AActor* Actor)
{
    ObjectPathStringProperty->SetValue(Actor->GetPathName());
}

FReply FBangoScriptObjectPathCustomization::OnClicked_UnlockButton()
{
    EAppReturnType::Type Return = FMessageDialog::Open(
        EAppMsgType::OkCancel,
        LOCTEXT("ObjectPath_UnlockEdit", "You should only edit this if you suspect it is corrupt! Would you like to continue?")
        );

    switch (Return)
    {
        case EAppReturnType::Ok:
        {
            // bUnlocked = true;
            PathStringPropertyValueWidget->SetEnabled(true);
        }
        default: { break; }
    }
    
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE