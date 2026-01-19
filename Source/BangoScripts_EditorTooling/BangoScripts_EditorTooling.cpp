#include "BangoScripts_EditorTooling.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

TCustomShowFlag<EShowFlagShippingValue::ForceDisabled> FBangoScripts_EditorToolingModule::BangoScriptsShowFlag(TEXT("BangoScriptsShowFlag"), true, EShowFlagGroup::SFG_Developer, FText(INVTEXT("Bango Scripts")));

void FBangoScripts_EditorToolingModule::StartupModule()
{
    
}

void FBangoScripts_EditorToolingModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FBangoScripts_EditorToolingModule, BangoScripts_EditorTooling)