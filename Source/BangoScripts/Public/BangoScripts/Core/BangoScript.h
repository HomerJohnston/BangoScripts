// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#pragma once

#include "BangoScriptHandle.h"
#include "Templates/SubclassOf.h"

#include "BangoScript.generated.h"

class UBangoScriptValidator;
class UBangoScriptSubsystem;

namespace BangoNodeBuilder
{
	struct BangoPauseSleep_Internal;
	struct BangoSkipSleep_Internal;
	struct BangoLaunchSleep_Internal;
	struct BangoCancelSleep_Internal;
	struct BangoExecuteScript_Internal;
}

DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FWaitUntilDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLatentActionDelegate);
DECLARE_DYNAMIC_DELEGATE(FOnLatentActionTick);
DECLARE_DYNAMIC_DELEGATE(FOnLatentActionCompleted);

#define LOCTEXT_NAMESPACE "BangoScripts"

using DataValidationDelegate = TDelegate<EDataValidationResult(class FDataValidationContext& Context, const UBangoScript* ScriptInstance)>;

/**
 * A reusable script asset, to be used in Bango Script Actor Components (UBangoScriptActorComponent) or in other places.
 */
UCLASS(Blueprintable)
class BANGOSCRIPTS_API UBangoScript : public UObject 
{
    GENERATED_BODY()

    friend UBangoScriptValidator;
	friend UBangoScriptSubsystem;
	friend BangoNodeBuilder::BangoExecuteScript_Internal;
	friend BangoNodeBuilder::BangoCancelSleep_Internal;
	friend BangoNodeBuilder::BangoLaunchSleep_Internal;
	friend BangoNodeBuilder::BangoSkipSleep_Internal;
	friend BangoNodeBuilder::BangoPauseSleep_Internal;
	friend class UK2Node_BangoFinishScript;
	friend class UK2Node_BangoRunScript;
    
protected:
    // TODO: is this a bad decision? How else can I do this? Can I register things to keep scripts alive? Can I discover delegate subs in blueprints?
    /** By default, Bango will destroy script objects once they stop running any latent actions. You need to turn this on if a script needs to wait for external events, such as subscribing to a delegate of another Actor. */
    UPROPERTY(EditAnywhere)
    bool bPreventAutoDestroy = false;
    
#if WITH_EDITORONLY_DATA
	// This will be kept in sync with the FBangoScriptContainer's ScriptGuid and is used for undo/redo purposes and other sync
	UPROPERTY(VisibleAnywhere)
	FGuid ScriptGuid;
#endif
	
	/** Reference to the object that ran this script. */ // Assigned by the Bango Script Subsystem on run.
	UPROPERTY()
	TWeakObjectPtr<UObject> This;
	
	/** When adding a 'This' node to the script, it will use this specified output class. See also  */
	UPROPERTY(EditAnywhere, DisplayName = "'This' Class")
	TSubclassOf<UObject> This_ClassType;
	
protected:
    bool GetKeepAliveWhenIdle() const { return bPreventAutoDestroy; }
    
public:
#if WITH_EDITOR
	FGuid GetScriptGuid() const { return ScriptGuid; }
	
	TSubclassOf<UObject> GetThis_ClassType() const { return This_ClassType; }
	
	void SetThis_ClassType(UClass* Class) { This_ClassType = Class; }
#endif
	    
protected:
    /** This is implemented by designers. */
    UFUNCTION(BlueprintImplementableEvent)
    void Start();

    /** This is supposed to be called at the end of the Execute function */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "Script", BlueprintProtected))
    static void Finish(UBangoScript* Script);

#if WITH_EDITOR
    bool ImplementsGetWorld() const override { return true; }
#endif
    
public:
	/** This is used by the script subsystem to respond to scripts finishing. */
	TMulticastDelegate<void(FBangoScriptHandle)> OnFinish_Native;
	
protected:
	
	/** This is used by the RunScript K2 node to continue exec when a script is finished. */
    UPROPERTY(BlueprintAssignable)
    FOnFinishDelegate OnFinishDelegate;

    UPROPERTY(Transient)
    FBangoScriptHandle Handle;

    UPROPERTY(Transient)
    TMap<int32, FOnLatentActionTick> SleepTickDelegates;
    
	UPROPERTY(Transient)
    TMap<int32, FOnLatentActionCompleted> SleepCancelDelegates;

    UFUNCTION(BlueprintCallable, Category="Bango|Delay", meta = (WorldContext="WorldContextObject", LatentInfo="LatentInfo", Duration="1.23", Keywords="sleep"))
    static UPARAM(DisplayName = "UUID") int32 LaunchSleep_Internal(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo, FOnLatentActionTick TickDelegate, FOnLatentActionCompleted CompleteDelegate);

    UFUNCTION(BlueprintCallable, Category="Bango|Delay", meta = (WorldContext="WorldContextObject", Keywords="sleep"))
    static void CancelSleep_Internal(UObject* WorldContextObject, int32 ActionUUID);

    UFUNCTION(BlueprintCallable, Category="Bango|Delay", meta = (WorldContext="WorldContextObject", Keywords="sleep"))
    static void SkipSleep_Internal(UObject* WorldContextObject, int32 ActionUUID);

    UFUNCTION(BlueprintCallable, Category="Bango|Delay", meta = (WorldContext="WorldContextObject", Keywords="sleep"))
    static void SetSleepPause_Internal(UObject* WorldContextObject, bool bPaused, int32 ActionUUID);
    
    UFUNCTION(BlueprintCallable, Category="Bango|Utilities", BlueprintPure)
    static UPARAM(DisplayName="Val") float Rand(float Hi, float Lo);
    
#if WITH_EDITOR
    
    friend class UBangoScriptValidator;

    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
    
protected:
    static DataValidationDelegate OnScriptRequestValidation;
#endif
};

#undef LOCTEXT_NAMESPACE