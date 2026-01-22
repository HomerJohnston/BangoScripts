#pragma once

#include "BangoScripts/Components/BangoScriptComponent.h"

#include "BangoLevelScriptsEditorSubsystem.generated.h"

class UObject;

using namespace Bango;

// ==============================================

namespace Bango
{
	struct FScriptContainerKey
	{
		FScriptContainerKey(TWeakObjectPtr<UObject> InScriptOuter, FBangoScriptContainer* InScriptContainer)
			: ScriptOuter(InScriptOuter)
			, ScriptContainer(InScriptContainer)
		{
			Script = ScriptContainer->GetScriptClass();
			UObject* Outer = InScriptOuter.Get(true);
		}
		
		// ----------------------------
		
		TWeakObjectPtr<UObject> ScriptOuter;
		FString OuterObjectPath;
		FBangoScriptContainer* ScriptContainer;
		TSoftClassPtr<UBangoScript> Script;

		// ----------------------------
		
		bool operator==(const FScriptContainerKey& Other) const
		{
			if (ScriptContainer->GetScriptClass().IsNull())
			{
				return ScriptOuter == Other.ScriptOuter && ScriptContainer == Other.ScriptContainer;
			}
			
			return ScriptContainer->GetScriptClass() == Other.ScriptContainer->GetScriptClass();
		}

		// ----------------------------
		
		friend int32 GetTypeHash(const FScriptContainerKey& ScriptContainerKey)
		{
			if (ScriptContainerKey.ScriptContainer->GetScriptClass().IsNull())
			{
				return HashCombine(ScriptContainerKey.ScriptOuter.GetWeakPtrTypeHash(), GetTypeHash(ScriptContainerKey.ScriptContainer));
			}

			return GetTypeHash(ScriptContainerKey.ScriptContainer->GetScriptClass().ToString());
		}
	};
}

// ==============================================

class IContentBrowserHideFolderIfEmptyFilter;

// ==============================================

/**
 * This subsystem is responsible for managing level script .uassets. 
 */
UCLASS()
class UBangoLevelScriptsEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	static UBangoLevelScriptsEditorSubsystem* Get();
	
protected:
	bool bMapLoading = false;
	bool bDuplicatingActors = false;

public:
	// Simple solution to force any active FBangoScriptContainerCustomization instances to redraw
	TMulticastDelegate<void()> OnScriptGenerated;
	
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void Deinitialize() override;
	
	void OnObjectTransacted(UObject* Object, const class FTransactionObjectEvent& TransactionEvent);
	
	void OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap);
	
	void OnMapOpened(const FString& String, bool bArg);
	
	void OnObjectRenamed(UObject* RenamedObject, UObject* RenamedObjectOuter, FName OldName) const;
	
	static TSharedPtr<IContentBrowserHideFolderIfEmptyFilter> Filter;	
	
	void OnLevelScriptContainerCreated(UObject* Outer, FBangoScriptContainer* ScriptContainer, FString BlueprintName = "");

	void OnLevelScriptContainerDestroyed(UObject* Outer, FBangoScriptContainer* ScriptContainer);

	void OnLevelScriptContainerDuplicated(UObject* Outer, FBangoScriptContainer* ScriptContainer, FString BlueprintName = "");
	
	// ------------------------------------------
	// Level script creation functions
private:
	void EnqueueCreatedScriptContainer(UObject* Owner, FBangoScriptContainer* ScriptContainer);
	
	void EnqueueDestroyedScriptContainer(UObject* Owner, FBangoScriptContainer* ScriptContainer);

	void RequestScriptQueueProcessing();
	
	// Master queue processing function
	void ProcessScriptRequestQueues();
	
	// Script creation methods
	void ProcessCreatedScriptRequest(TWeakObjectPtr<UObject> Owner, FBangoScriptContainer* ScriptContainer);
	
	void CreateLevelScript(UObject* Outer, FBangoScriptContainer* ScriptContainer);
	
	void DuplicateLevelScript(UObject* Outer, FBangoScriptContainer* ScriptContainer);
	
	void TryUndeleteScript(FSoftObjectPath ScriptClassSoft, FBangoScriptContainer* ScriptContainer);
	
	// Script destruction method
	void ProcessDestroyedScriptRequest(TSoftClassPtr<UBangoScript> ScriptClass);
	
private:
	TSet<FScriptContainerKey> CreationRequests;
	
	TSet<TSoftClassPtr<UBangoScript>> DestructionRequests;
	
	FTimerHandle ProcessScriptRequestQueuesHandle;
	
};
