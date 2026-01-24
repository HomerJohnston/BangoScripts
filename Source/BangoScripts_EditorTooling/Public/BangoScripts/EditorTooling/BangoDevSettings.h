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

	// ------------------------------------------
	// Blueprint settings
protected:
	/*
	 * By default, opening a Bango Script Blueprint editor will disable slate throttling. This is because UDebugDrawService, which is used to draw connection lines in the level editor viewport, will flash distractingly during slate throttling.
	 * (slate throttling is where the editor chokes out non-critical processes such as rendering the level viewport in order to keep editor slate as responsive as possible)
	 */
	UPROPERTY(Category = "Bango", EditDefaultsOnly, Config)
	bool bPreventSlateThrottlingOverrides = false;
	
	/** Past this distance, any debug labels will be strongly faded or invisible. Unset this to always render, e.g. for isometric views. */
	UPROPERTY(Config, EditAnywhere, Category = "Bango", meta = (ClampMin = 0.0, ClampMax = 100000.0, UIMin = 0.0, UIMax = 50000.0))
	TOptional<float> ScriptIconPIEDisplayDistance = 10000;

public:
	static bool GetPreventSlateThrottlingOverrides();
	
	static float GetPIEDisplayDistance();

	// void OnCvarChange();
};
