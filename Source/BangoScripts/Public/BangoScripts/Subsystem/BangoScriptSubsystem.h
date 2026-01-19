#pragma once

#include "BangoScripts/Components/BangoScriptComponent.h"
#include "BangoScripts/Core/BangoScriptHandle.h"
#include "BangoScripts/Utility/ObjectTicker.h"
#include "Subsystems/WorldSubsystem.h"

#include "BangoScriptSubsystem.generated.h"

struct FStreamableHandle;
struct FBangoScriptHandle;
class UBangoScript;

// ----------------------------------------------

// TODO I need to implement a way for these to be loaded and played forcefully immediately
// TODO I need a way for things upstream to initiate async loading sooner - maybe script components and triggers should always keep their scripts loaded
USTRUCT()
struct FBangoQueuedScript
{
	GENERATED_BODY()
	
	FBangoQueuedScript() = default;
	
	FBangoQueuedScript(UObject* InRunner, const FInstancedPropertyBag* InPropertyBag, TSoftClassPtr<UBangoScript> InScriptClass, FBangoScriptHandle InHandle)	
		: Runner(InRunner), PropertyBag(InPropertyBag), ScriptClass(InScriptClass), Handle(InHandle) { }
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> Runner;

	// MUST be contained on the Runner, for safety. This is uncontrolled. TODO is there a nicer way to architect this?
	const FInstancedPropertyBag* PropertyBag;
	
	UPROPERTY(Transient)
	TSoftClassPtr<UBangoScript> ScriptClass;
	
	UPROPERTY(Transient)
	FBangoScriptHandle Handle;
	
	TSharedPtr<FStreamableHandle> ScriptClassHandle;
	
	void LoadAsync();

	void LoadSync();

	bool IsReadyToRun();
};

// ----------------------------------------------

UCLASS()
class UBangoScriptSubsystem : public UWorldSubsystem, public TObjectTicker<UBangoScriptSubsystem>
{
	GENERATED_BODY()

protected:
	static UBangoScriptSubsystem* Get(UObject* WorldContext);

protected:
	UPROPERTY(Transient)
	TArray<FBangoQueuedScript> QueuedScripts;
	
	UPROPERTY(Transient)
	TMap<FBangoScriptHandle, TObjectPtr<UBangoScript>> RunningScripts;

	TMulticastDelegate<void(FBangoScriptHandle)> OnScriptFinished;
	
	bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

	// Normal usage path
	UFUNCTION(BlueprintCallable, Category = "Bango|Scripts", meta = (BlueprintInternalUseOnly = "true"))
	static FBangoScriptHandle K2_EnqueueScript(TSoftClassPtr<UBangoScript> ScriptClass, UObject* Runner, bool bLoadImmediately = false);
	
	// Alternate usage for cases where I want external things to supply the handle - for example the ScriptComponent does this so that it can 
	static FBangoScriptHandle EnqueueScript(TSoftClassPtr<UBangoScript> ScriptClass, UObject* Runner, const FInstancedPropertyBag* PropertyBag, bool bLoadImmediately = false);
	
	static void AbortScript(UObject* Requester, FBangoScriptHandle& Handle);
	
	static void RegisterOnScriptFinished(UObject* WorldContext, FBangoScriptHandle RunningHandle, const TDelegate<void(FBangoScriptHandle)>& Delegate);
	
protected:
	void Tick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	
	void PruneFinishedScripts(UWorld* World);

	void PruneQueuedInvalidRunnerScripts();
	
	void LoadQueuedScripts();

	void LaunchQueuedScripts();
	
	void TransferPropertyBagToScriptInstance(const FInstancedPropertyBag* PropertyBag, UBangoScript* Script);
	
	void RegisterScript(UBangoScript* ScriptInstance);

	void UnregisterScript(UObject* WorldContext, FBangoScriptHandle& Handle);
};
