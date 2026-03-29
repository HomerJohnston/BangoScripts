#pragma once

#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"

class FBangoScriptObjectPathCustomization : public IPropertyTypeCustomization
{
    // ==========================================
    // CONSTRUCTION
public:
    
	FBangoScriptObjectPathCustomization();
	
	virtual ~FBangoScriptObjectPathCustomization() override;
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    // ==========================================
    // STATE

protected:
    TSharedPtr<IPropertyHandle> ObjectPathStringProperty;
    
    TSharedPtr<SWidget> PathStringPropertyValueWidget;

    // ==========================================
    // API

protected:
    
	void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    // ==========================================
    // HELPERS
    
protected:
    
    void GetAllowedClasses(TArray<const UClass*>& AllowedClasses);
    
    bool ShouldFilterActor(const AActor* Actor);
    
    void ActorSelectedFromPicker(AActor* Actor);
    
    FReply OnClicked_UnlockButton();
};
