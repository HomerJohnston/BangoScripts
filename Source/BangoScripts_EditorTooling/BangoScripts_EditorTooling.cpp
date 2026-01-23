#include "BangoScripts_EditorTooling.h"

#include "BangoScripts/EditorTooling/BangoDebugUtility.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

TCustomShowFlag<EShowFlagShippingValue::ForceDisabled> FBangoScripts_EditorToolingModule::BangoScriptsShowFlag( Bango::Debug::ScriptsShowFlagName(), true, EShowFlagGroup::SFG_Developer, FText(INVTEXT("Bango Scripts")));

void FBangoScripts_EditorToolingModule::StartupModule()
{
    
}

void FBangoScripts_EditorToolingModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FBangoScripts_EditorToolingModule, BangoScripts_EditorTooling)