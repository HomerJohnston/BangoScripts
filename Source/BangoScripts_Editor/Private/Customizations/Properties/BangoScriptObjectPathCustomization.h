#pragma once

#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"

class FBangoScriptObjectPathCustomization : public IPropertyTypeCustomization
{
public:
    
	FBangoScriptObjectPathCustomization();
	
	virtual ~FBangoScriptObjectPathCustomization() override;
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	// ------------------------------------------
protected:

    TSharedPtr<IPropertyHandle> ObjectPathStringProperty;
    TSharedPtr<SWidget> PathStringPropertyValueWidget;
    
    bool bUnlocked = false;
    
	// ------------------------------------------

	void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	// ------------------------------------------

	void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    
    
    // ------------------------------------------
protected:
    void GetAllowedClasses(TArray<const UClass*>& AllowedClasses);
    
    bool ShouldFilterActor(const AActor* Actor);
    
    void ActorSelectedFromPicker(AActor* Actor);
    
    FReply OnClicked_UnlockButton();
};
