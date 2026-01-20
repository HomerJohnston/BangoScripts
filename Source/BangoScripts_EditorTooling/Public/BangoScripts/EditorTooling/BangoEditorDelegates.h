#pragma once

#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "Misc/Guid.h"
#include "UObject/SoftObjectPtr.h"

class UObject;
struct FBangoScriptContainer;
class UBangoScriptBlueprint;
class UBangoScript;
class UBangoScriptComponent;
struct FBangoDebugDrawCanvas;

#if WITH_EDITOR
struct FBangoEditorDelegates
{
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UObject* Outer, FBangoScriptContainer* ScriptContainer, FString Name)> OnScriptContainerCreated;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UObject* Outer, FBangoScriptContainer* ScriptContainer, FString Name)> OnScriptContainerDuplicated;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UObject* Outer, FBangoScriptContainer* ScriptContainer)> OnScriptContainerDestroyed;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UObject* Outer, TSoftClassPtr<UBangoScript> ScriptContainer)> OnScriptContainerUnregisteredDuringTransaction;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(FGuid ScriptID, UBangoScriptBlueprint*& FoundBlueprint)> OnBangoActorComponentUndoDelete;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(AActor* Actor)> RequestNewID;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UBangoScriptComponent* ScriptComponent)> OnScriptComponentClicked;
	
	//  
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(FBangoDebugDrawCanvas& DebugDrawData, bool bPIE)> DebugDrawRequest;

	// FBangoScriptContainerCustomization instances subscribe to this to regenerate themselves when the script subsystem does things to level scripts
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void()> OnScriptGenerated;
	
	// Used by FBangoBlueprintEditor to automatically start debugging newly ran scripts
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UBangoScript* ScriptClass)> OnBangoScriptRan;
	
	// Used by FBangoBlueprintEditor
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UBangoScript* ScriptClass)> OnBangoScriptFinished;
};
#endif
