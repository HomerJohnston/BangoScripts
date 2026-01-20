#pragma once

#include "CoreMinimal.h"
#include "ShowFlags.h"
#include "Modules/ModuleManager.h"

class FBangoScripts_EditorToolingModule : public IModuleInterface
{
public:
	BANGOSCRIPTS_EDITORTOOLING_API static TCustomShowFlag<EShowFlagShippingValue::ForceDisabled> BangoScriptsShowFlag;

public:
    void StartupModule() override;
    void ShutdownModule() override;
};
