#include "BangoScripts/Subsystem/BangoScriptSubsystem.h"

#include "BangoScripts/Core/BangoScriptHandle.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Utility/BangoScriptsLog.h"
#include "Engine/AssetManager.h"
#include "Engine/LatentActionManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

// ==============================================
// FBangoQueuedScript

void FBangoQueuedScript::LoadAsync()
{
	// Already loading or loaded
	if (ScriptClassHandle.IsValid())
	{
		return;
	}

	// Forget about it
	if (!Runner.IsValid())
	{
		return;
	}
	
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	ScriptClassHandle = Streamable.RequestAsyncLoad(ScriptClass.ToSoftObjectPath());
}

// ----------------------------------------------

void FBangoQueuedScript::LoadSync()
{
	// Already loading or loaded
	if (ScriptClassHandle.IsValid())
	{
		return;
	}

	// Forget about it
	if (!Runner.IsValid())
	{
		return;
	}
	
	UE_LOG(LogBango, Warning, TEXT("Loading Bango script synchronously. You should consider preloading this earlier. Runner: %s, Script: %s"), *Runner->GetName(), *ScriptClass->GetName());
	
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.LoadSynchronous(ScriptClass.ToSoftObjectPath(), false, &ScriptClassHandle);
}

// ----------------------------------------------

bool FBangoQueuedScript::IsReadyToRun()
{
	return Runner.IsValid() && ScriptClass.IsValid();
}

// ==============================================
// UBangoScriptSubsystem

UBangoScriptSubsystem* UBangoScriptSubsystem::Get(UObject* WorldContext)
{
	if (IsValid(WorldContext))
	{
		return WorldContext->GetWorld()->GetSubsystem<UBangoScriptSubsystem>();
	}

	return nullptr;
}

// ----------------------------------------------

bool UBangoScriptSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// ----------------------------------------------

void UBangoScriptSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	TickFunction.RegisterTickFunction(GetWorld()->PersistentLevel);
}

// ----------------------------------------------

FBangoScriptHandle UBangoScriptSubsystem::K2_EnqueueScript(TSoftClassPtr<UBangoScript> ScriptClass, UObject* Runner, bool bLoadImmediately)
{
	if (!Runner)
	{
		UE_LOG(LogBango, Error, TEXT("RunScript called with null runner!"));
		return FBangoScriptHandle::GetNullHandle();
	}

	if (ScriptClass.IsNull())
	{
		UE_LOG(LogBango, Warning, TEXT("RunScript called with null script!"));
		return FBangoScriptHandle::GetNullHandle();
	}

	UBangoScriptSubsystem* Subsystem = Get(Runner);
	
	if (!Subsystem)
	{
		UE_LOG(LogBango, Error, TEXT("RunScript called but could not find UBangoScriptSubsystem!"));
		return FBangoScriptHandle::GetNullHandle();
	}
	
	return EnqueueScript(ScriptClass, Runner, nullptr, bLoadImmediately);
}

FBangoScriptHandle UBangoScriptSubsystem::EnqueueScript(TSoftClassPtr<UBangoScript> ScriptClass, UObject* Runner, const FInstancedPropertyBag* PropertyBag, bool bLoadImmediately)
{
	check(Runner);
	
	UBangoScriptSubsystem* Subsystem = Get(Runner);
	check(Subsystem);
	
	FBangoScriptHandle NewHandle = FBangoScriptHandle::NewHandle();
	
	FBangoQueuedScript QueuedScript { Runner, PropertyBag, ScriptClass, NewHandle };
	
	if (bLoadImmediately)
	{
		QueuedScript.LoadSync();
	}
	
	Subsystem->QueuedScripts.Add( MoveTemp(QueuedScript) );
	
	return NewHandle;
}

// ----------------------------------------------

void UBangoScriptSubsystem::AbortScript(UObject* Requester, FBangoScriptHandle& Handle)
{
	if (!Handle.IsRunning())
	{
		UE_LOG(LogBango, Warning, TEXT("AbortScript called with invalid script handle!"));
		return;
	}
	
	if (!IsValid(Requester))
	{
		UE_LOG(LogBango, Warning, TEXT("AbortScript called with null requester! A requester is necessary for world context."));
		return;
	}

	UWorld* World = Requester->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		UE_LOG(LogBango, Warning, TEXT("AbortScript called with invalid requested not in game world!"));
		return;
	}
	
	UBangoScriptSubsystem* Subsystem = UBangoScriptSubsystem::Get(Requester);
	
	if (!Subsystem)
	{
		UE_LOG(LogBango, Warning, TEXT("AbortScript called but could not get subsytem, unknown error!"));
		return;
	}
	
	Subsystem->UnregisterScript(Requester, Handle);
	Handle.Invalidate();
}

void UBangoScriptSubsystem::RegisterOnScriptFinished(UObject* WorldContext, FBangoScriptHandle RunningHandle, const TDelegate<void(FBangoScriptHandle)>& Delegate)
{
	check(WorldContext);
	UBangoScriptSubsystem* Subsystem = Get(WorldContext);
	check(Subsystem);
	
	auto QueuedScriptContainsHandle = [RunningHandle] (const FBangoQueuedScript& QueuedScript) -> bool
	{
		return QueuedScript.Handle == RunningHandle;
	};
	
	if (Subsystem->RunningScripts.Contains(RunningHandle) || Subsystem->QueuedScripts.ContainsByPredicate(QueuedScriptContainsHandle))
	{
		Subsystem->OnScriptFinished.Add(Delegate);
	}
}

// ----------------------------------------------

void UBangoScriptSubsystem::Tick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	UWorld* World = GetWorld();
	
	if (!World || !World->IsGameWorld())
	{
		UE_LOG(LogBango, Warning, TEXT("Bango Script Subsytem tried to tick outside of game world!"));
		return;
	}

	PruneFinishedScripts(World);

	PruneQueuedInvalidRunnerScripts();
	
	LoadQueuedScripts();
	
	LaunchQueuedScripts();
}

void UBangoScriptSubsystem::PruneFinishedScripts(UWorld* World)
{
	FLatentActionManager& Manager = World->GetLatentActionManager();

	TArray<FBangoScriptHandle> DeadScriptHandles;
	
	for (auto [Handle, Script] : RunningScripts)
	{
		if (Manager.GetNumActionsForObject(Script) == 0)
		{
			UE_LOG(LogBango, Verbose, TEXT("Idle script being automatically destroyed: {%s}"), *Script->GetName());
			DeadScriptHandles.Add(Handle);
		}
	}
	
	for (FBangoScriptHandle& Handle : DeadScriptHandles)
	{
		UnregisterScript(this, Handle);
	}	
}

void UBangoScriptSubsystem::PruneQueuedInvalidRunnerScripts()
{
	for (int32 i = 0; i < QueuedScripts.Num(); ++i)
	{
		FBangoQueuedScript& QueuedScript = QueuedScripts[i];
		
		// If runner has become invalid, remove this script from the queue
		if (!QueuedScript.Runner.IsValid())
		{
			QueuedScripts.RemoveAtSwap(i, EAllowShrinking::No);
			--i;
		}
	}
}

void UBangoScriptSubsystem::LoadQueuedScripts()
{
	for (FBangoQueuedScript& QueuedScript : QueuedScripts)
	{
		QueuedScript.LoadAsync();
	}
}

void UBangoScriptSubsystem::LaunchQueuedScripts()
{
	bool bScriptsRan = false;
	
	for (int32 i = 0; i < QueuedScripts.Num(); ++i)
	{
		FBangoQueuedScript& QueuedScript = QueuedScripts[i];
		
		if (QueuedScript.IsReadyToRun())
		{
			UObject* Runner = QueuedScript.Runner.Get();
			check(Runner);
			
			TSubclassOf<UBangoScript> ScriptClass = QueuedScript.ScriptClass.Get();
			check(ScriptClass);
			
			UBangoScript* NewScriptInstance = NewObject<UBangoScript>(Runner, ScriptClass);
			NewScriptInstance->Handle = QueuedScript.Handle;
			NewScriptInstance->This = Runner;
			
			if (QueuedScript.PropertyBag)
			{
				TransferPropertyBagToScriptInstance(QueuedScript.PropertyBag, NewScriptInstance);
			}
			
			RegisterScript(NewScriptInstance);
			
			QueuedScripts.RemoveAtSwap(i, EAllowShrinking::No);
			--i;
		}
	}
	
	// microoptimizations yay! let's just not care if theres 10 slots for queued scripts. Most of the time there will probably be 2 or 3.
	if (bScriptsRan && QueuedScripts.GetSlack() > 10)
	{
		QueuedScripts.Shrink();
	}
}

void* ContainerVoidPtrToValuePtrInternal(const void* ContainerPtr, int32 Offset_Internal, int32 ElementSize, int32 ArrayIndex = 0)
{	
	return (uint8*)ContainerPtr + Offset_Internal + static_cast<size_t>(ElementSize) * ArrayIndex;
}

void UBangoScriptSubsystem::TransferPropertyBagToScriptInstance(const FInstancedPropertyBag* PropertyBag, UBangoScript* Script)
{
    const UPropertyBag* PropertyBagStruct = PropertyBag->GetPropertyBagStruct();

    if (!IsValid(PropertyBagStruct))
    {
        // If no properties have been added this will be nullptr
        return;
    }
        
	FConstStructView BagView = PropertyBag->GetValue(); // a view of the actual bag data
	const void* BagMemoryPtr = BagView.GetMemory(); // the actual memory address of the bag data
	
	// for each property in the UScriptStruct definition of the FInstancedPropertyBag
	for (const FPropertyBagPropertyDesc& PropertyDesc : PropertyBagStruct->GetPropertyDescs())
	{
		const FName ScriptPropertyName = PropertyDesc.Name;
		
		FProperty* ScriptProperty = Script->GetClass()->FindPropertyByName(ScriptPropertyName);
		const FProperty* BagProperty = PropertyDesc.CachedProperty;
		
		if (ScriptProperty && BagProperty)
		{
			int32 Offset = BagProperty->GetOffset_ForInternal();
			int32 ElementSize = BagProperty->GetElementSize();
			
			void* BagValue = ContainerVoidPtrToValuePtrInternal(BagMemoryPtr, Offset, ElementSize, 0);
			void* ScriptValue = ScriptProperty->ContainerPtrToValuePtr<void>(Script);

			ScriptProperty->CopySingleValue(ScriptValue, BagValue);
		}
	}
}

// ----------------------------------------------

void UBangoScriptSubsystem::RegisterScript(UBangoScript* ScriptInstance)
{
	UBangoScriptSubsystem* Subsystem = Get(ScriptInstance);
	check(Subsystem->GetWorld()->HasBegunPlay());

	Subsystem->RunningScripts.Add(ScriptInstance->Handle, ScriptInstance);
	
	UE_LOG(LogBango, Verbose, TEXT("Script running: {%s}"), *ScriptInstance->GetName());
	
#if WITH_EDITOR
	FBangoEditorDelegates::OnBangoScriptRan.Broadcast(ScriptInstance);
#endif
	
	ScriptInstance->Start();
}

// ----------------------------------------------

void UBangoScriptSubsystem::UnregisterScript(UObject* WorldContext, FBangoScriptHandle& Handle)
{
	if (!Handle.IsRunning())
	{
		return;
	}
	
	UBangoScriptSubsystem* Subsystem = Get(WorldContext);

	TObjectPtr<UBangoScript> ScriptInstance;
	
	if (Subsystem->RunningScripts.RemoveAndCopyValue(Handle, ScriptInstance))
	{
		UE_LOG(LogBango, Verbose, TEXT("Script halting: {%s}"), *ScriptInstance->GetName());

#if WITH_EDITOR
		FBangoEditorDelegates::OnBangoScriptFinished.Broadcast(ScriptInstance);
#endif
		
		UBangoScript::Finish(ScriptInstance);
	}


	OnScriptFinished.Broadcast(Handle);
	
	Handle.Invalidate();
}

// ----------------------------------------------

#undef LOCTEXT_NAMESPACE