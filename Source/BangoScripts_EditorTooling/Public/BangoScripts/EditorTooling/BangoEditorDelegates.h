#pragma once

#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "Misc/Guid.h"
#include "UObject/SoftObjectPtr.h"

class UTexture2D;
class IBangoScriptHolderInterface;
class UObject;
struct FBangoScriptContainer;
class UBangoScriptBlueprint;
class UBangoScript;
class UBangoScriptComponent;
struct FBangoDebugDrawCanvas;

enum class EBangoScriptComponentRegisterStatus : uint8
{
	Registered,
	Unregistered
};

#if WITH_EDITOR
struct FBangoEditorDelegates
{
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(IBangoScriptHolderInterface& ScriptHolder, FString RequestedBlueprintName)> OnScriptContainerCreated;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(IBangoScriptHolderInterface& ScriptHolder)> OnScriptContainerDuplicated;
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(IBangoScriptHolderInterface& ScriptHolder)> OnScriptContainerDestroyed;
	
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
	
	// 
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(IBangoScriptHolderInterface* Requester, const TSoftObjectPtr<UTexture2D>& OverlayTexture)> OnCustomBillboardRequested;
	
	//
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(UBangoScriptComponent* ScriptComponent, EBangoScriptComponentRegisterStatus RegistrationStatus)> ScriptComponentRegistered;
	
	// Can be broadcasted by game code (e.g. UBangoScriptComponent's PreSave func) to initiate an editor save of a script asset
	BANGOSCRIPTS_EDITORTOOLING_API static TMulticastDelegate<void(TSoftClassPtr<UBangoScript> UnsavedScript)> RequestScriptSave;
};
#endif
