

#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"

TMulticastDelegate<void(UObject* Outer, FBangoScriptContainer* ScriptContainer, FString Name)> FBangoEditorDelegates::OnScriptContainerCreated;

TMulticastDelegate<void(UObject* Outer, FBangoScriptContainer* ScriptContainer, FString Name)> FBangoEditorDelegates::OnScriptContainerDuplicated;

TMulticastDelegate<void(UObject* Outer, FBangoScriptContainer* ScriptContainer)> FBangoEditorDelegates::OnScriptContainerDestroyed;

TMulticastDelegate<void(UObject* Outer, TSoftClassPtr<UBangoScript> ScriptContainer)> FBangoEditorDelegates::OnScriptContainerUnregisteredDuringTransaction;

TMulticastDelegate<void(FGuid ScriptID, UBangoScriptBlueprint*& FoundBlueprint)> FBangoEditorDelegates::OnBangoActorComponentUndoDelete;

TMulticastDelegate<void(AActor* Actor)> FBangoEditorDelegates::RequestNewID;

TMulticastDelegate<void(UBangoScriptComponent* ScriptComponent)> FBangoEditorDelegates::OnScriptComponentClicked;

TMulticastDelegate<void(FBangoDebugDrawCanvas& DebugDrawData, bool bPIE)> FBangoEditorDelegates::DebugDrawRequest;

TMulticastDelegate<void()> FBangoEditorDelegates::OnScriptGenerated;

TMulticastDelegate<void(UBangoScript* ScriptClass)> FBangoEditorDelegates::OnBangoScriptRan;

TMulticastDelegate<void(UBangoScript* ScriptClass)> FBangoEditorDelegates::OnBangoScriptFinished;

