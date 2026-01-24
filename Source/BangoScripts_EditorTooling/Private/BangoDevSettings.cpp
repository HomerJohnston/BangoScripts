// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#include "BangoScripts/EditorTooling/BangoDevSettings.h"

#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<bool> CVarBangoShowEventsInGame(
	TEXT("Bango.ShowEventsInGame"),
	false,
	TEXT("TODO Test"));

bool UBangoScriptsDeveloperSettings::GetPreventSlateThrottlingOverrides()
{
	return Get().bPreventSlateThrottlingOverrides;
}

/*
void UBangoScriptsDeveloperSettings::PostCDOContruct()
{
	Super::PostCDOContruct();
	
	static const auto ShowEventsInGame = IConsoleManager::Get().FindConsoleVariable(TEXT("Bango.ShowEventsInGame"));

	ShowEventsInGame->Set(GetShowEventsInGame());

	FAutoConsoleVariableSink CVarSink(FConsoleCommandDelegate::CreateUObject(this, &ThisClass::OnCvarChange));
}

bool UBangoScriptsDeveloperSettings::GetShowEventsInGame() const
{
	return bShowEventsInGame;
}

float UBangoScriptsDeveloperSettings::GetFarDisplayDistance() const
{
	return ScriptIconPIEDisplayDistance;
}

float UBangoScriptsDeveloperSettings::GetNearDisplayDistance() const
{
	return NearDisplayDistance;
}

float UBangoScriptsDeveloperSettings::GetEventDisplaySize() const
{
	return EventDisplaySize;
}

void UBangoScriptsDeveloperSettings::OnCvarChange()
{	
	const IConsoleVariable* ShowInGameCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Bango.ShowEventsInGame"));

	if (bShowEventsInGame != ShowInGameCVar->GetBool())
	{
		UE_LOG(LogBango, Display, TEXT("Updating CVar Bango.ShowEventsInGame; if you would like your setting to persist between editor startups, change it in Editor Preferences."))
		bShowEventsInGame = ShowInGameCVar->GetBool();
	}
}

*/
float UBangoScriptsDeveloperSettings::GetPIEDisplayDistance()
{
	const TOptional<float>& Val = Get().ScriptIconPIEDisplayDistance;
		
	if (Val.IsSet())
	{
		return Val.GetValue();
	}
		
	return -1.0f;
}
