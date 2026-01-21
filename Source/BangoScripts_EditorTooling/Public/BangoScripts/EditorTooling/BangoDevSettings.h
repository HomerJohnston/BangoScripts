// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "BangoDevSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, DisplayName="Bango Scripts")
class BANGOSCRIPTS_EDITORTOOLING_API UBangoScriptsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	// void PostCDOContruct() override;	

    static const UBangoScriptsDeveloperSettings& Get()
    {
        const UBangoScriptsDeveloperSettings* CDO = GetDefault<UBangoScriptsDeveloperSettings>();
        check(CDO);
        
        return *CDO;
    }
    
    // ------------------------------------------
    // SETTINGS
protected:

    UPROPERTY(Config, EditAnywhere, Category = "Bango")
	bool bShowEventsInGame = false;
	
	/** Beyond this distance, all event data will be hidden */
	UPROPERTY(Config, EditAnywhere, Category = "Bango")
	float ScriptIconPIEDisplayDistance = 10000;

public:
	static float GetPIEDisplayDistance()
	{
	    return Get().ScriptIconPIEDisplayDistance;
	};

	// void OnCvarChange();

#if WITH_EDITOR
	// void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
