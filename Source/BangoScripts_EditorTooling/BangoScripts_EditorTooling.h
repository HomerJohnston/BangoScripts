#pragma once

#include "CoreMinimal.h"
#include "ShowFlags.h"
#include "Modules/ModuleManager.h"

class FBangoScripts_EditorToolingModule : public IModuleInterface
{
	static TCustomShowFlag<EShowFlagShippingValue::ForceDisabled> BangoScriptsShowFlag;

public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
