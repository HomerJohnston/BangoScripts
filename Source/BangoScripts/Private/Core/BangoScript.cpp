#include "BangoScripts/Core/BangoScript.h"

#include "BangoScripts/Core/BangoScriptHandle.h"
#include "BangoScripts/LatentActions/BangoSleepAction.h"
#include "BangoScripts/Subsystem/BangoScriptSubsystem.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Engine/Engine.h"
#include "Misc/DataValidation.h"
#include "UObject/AssetRegistryTagsContext.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

#if WITH_EDITOR
DataValidationDelegate UBangoScript::OnScriptRequestValidation;
#endif

void UBangoScript::Finish(UBangoScript* Script)
{
    if (UWorld* World = GEngine->GetWorldFromContextObject(Script, EGetWorldErrorMode::LogAndReturnNull))
    {
        FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

        LatentActionManager.RemoveActionsForObject(Script);
    }
    
	Script->OnFinish_Native.Broadcast(Script->Handle);
    Script->OnFinishDelegate.Broadcast();
    Script->Handle.Invalidate();
	
	Script->MarkAsGarbage();
}

int32 UBangoScript::LaunchSleep_Internal(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo, FOnLatentActionTick BPDelayTickEvent, FOnLatentActionCompleted BPDelayCompleteEvent)
{
    int32 UUID = LatentInfo.UUID;
    
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
        if (!LatentActionManager.FindExistingAction<FBangoSleepAction>(LatentInfo.CallbackTarget, UUID))
        {
            FBangoSleepAction* SleepAction = new FBangoSleepAction(Duration, LatentInfo);
            LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, UUID, SleepAction);

            FOnLatentActionTick TickDelegate;
            TickDelegate = BPDelayTickEvent;
            
            SleepAction->OnTick.AddLambda([TickDelegate]()
            {
                TickDelegate.ExecuteIfBound();
            });

            FOnLatentActionCompleted CompleteDelegate;
            CompleteDelegate = BPDelayCompleteEvent;
            
            SleepAction->OnComplete.AddLambda([CompleteDelegate]()
            {
                CompleteDelegate.ExecuteIfBound();
            });

            return UUID;
        }
    }

    UE_LOG(LogBango, Warning, TEXT("Unknown error running LaunchSleep_Internal!"));
    return 0;
}

void UBangoScript::CancelSleep_Internal(UObject* WorldContextObject, int32 ActionUUID)
{
    if (ActionUUID == 0)
    {
        UE_LOG(LogBango, Warning, TEXT("CancelSleep_Internal called with ActionUUID {%i}"), ActionUUID);
        return;
    }
    
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
        FBangoSleepAction* Action = LatentActionManager.FindExistingAction<FBangoSleepAction>(WorldContextObject, ActionUUID);

        if (Action)
        {
            Action->Cancel();
        }
    }
}

void UBangoScript::SkipSleep_Internal(UObject* WorldContextObject, int32 ActionUUID)
{
    if (ActionUUID == 0)
    {
        UE_LOG(LogBango, Warning, TEXT("SkipSleep_Internal called with ActionUUID {%i}"), ActionUUID);
        return;
    }
    
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
        FBangoSleepAction* Action = LatentActionManager.FindExistingAction<FBangoSleepAction>(WorldContextObject, ActionUUID);

        if (Action)
        {
            Action->Skip();
        }
    }    
}

void UBangoScript::SetSleepPause_Internal(UObject* WorldContextObject, bool bPaused, int32 ActionUUID)
{
    if (ActionUUID == 0)
    {
        UE_LOG(LogBango, Warning, TEXT("SetSleepPause_Internal called with ActionUUID {%i}"), ActionUUID);
        return;
    }
    
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
        FBangoSleepAction* Action = LatentActionManager.FindExistingAction<FBangoSleepAction>(WorldContextObject, ActionUUID);

        if (Action)
        {
            Action->SetPaused(bPaused);
        }
    }    
}


#if WITH_EDITOR
EDataValidationResult UBangoScript::IsDataValid(class FDataValidationContext& Context) const
{
    if (OnScriptRequestValidation.IsBound())
    {
        return OnScriptRequestValidation.Execute(Context, this);        
    }
    
    return UObject::IsDataValid(Context);
}
#endif

#undef LOCTEXT_NAMESPACE