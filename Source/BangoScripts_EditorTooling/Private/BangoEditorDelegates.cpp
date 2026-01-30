

#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"

class IBangoScriptHolderInterface;

TMulticastDelegate<void(IBangoScriptHolderInterface& ScriptHolder, FString RequestedBlueprintName)> FBangoEditorDelegates::OnScriptContainerCreated;

TMulticastDelegate<void(IBangoScriptHolderInterface& ScriptHolder)> FBangoEditorDelegates::OnScriptContainerDuplicated;

TMulticastDelegate<void(IBangoScriptHolderInterface& ScriptHolder)> FBangoEditorDelegates::OnScriptContainerDestroyed;

TMulticastDelegate<void(AActor* Actor)> FBangoEditorDelegates::RequestNewID;

TMulticastDelegate<void(UBangoScriptComponent* ScriptComponent)> FBangoEditorDelegates::OnScriptComponentClicked;

TMulticastDelegate<void(FBangoDebugDrawCanvas& DebugDrawData, bool bPIE)> FBangoEditorDelegates::DebugDrawRequest;

TMulticastDelegate<void()> FBangoEditorDelegates::OnScriptGenerated;

TMulticastDelegate<void(UBangoScript* ScriptClass)> FBangoEditorDelegates::OnBangoScriptRan;

TMulticastDelegate<void(UBangoScript* ScriptClass)> FBangoEditorDelegates::OnBangoScriptFinished;

TMulticastDelegate<void(IBangoScriptHolderInterface* Requester, const TSoftObjectPtr<UTexture2D>& OverlayTexture)> FBangoEditorDelegates::OnCustomBillboardRequested;

TMulticastDelegate<void(UBangoScriptComponent* ScriptComponent, EBangoScriptComponentRegisterStatus RegistrationStatus)> FBangoEditorDelegates::ScriptComponentRegistered;