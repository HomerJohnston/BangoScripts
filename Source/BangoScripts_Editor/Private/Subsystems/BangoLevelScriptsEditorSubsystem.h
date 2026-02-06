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
		FScriptContainerKey(IBangoScriptHolderInterface& ScriptHolder)
			: Outer(ScriptHolder._getUObject())
			, ScriptContainer(&ScriptHolder.GetScriptContainer())
		{
			Script = ScriptContainer->GetScriptClass();
		}
		
		// ----------------------------
		
		TWeakObjectPtr<UObject> Outer;
		FString OuterObjectPath;
		FBangoScriptContainer* ScriptContainer;
		TSoftClassPtr<UBangoScript> Script;

		// ----------------------------
		
		bool operator==(const FScriptContainerKey& Other) const
		{
			if (ScriptContainer->GetScriptClass().IsNull())
			{
				return Outer == Other.Outer && ScriptContainer == Other.ScriptContainer;
			}
			
			return ScriptContainer->GetScriptClass() == Other.ScriptContainer->GetScriptClass();
		}

		// ----------------------------
		
		friend int32 GetTypeHash(const FScriptContainerKey& ScriptContainerKey)
		{
			if (ScriptContainerKey.ScriptContainer->GetScriptClass().IsNull())
			{
				return HashCombine(ScriptContainerKey.Outer.GetWeakPtrTypeHash(), GetTypeHash(ScriptContainerKey.ScriptContainer));
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
	bool bDuplicatingLevel = false;
	
	FDelegateHandle OnAssetAddedHandle;
	FDelegateHandle OnAssetRemovedHandle;

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void Deinitialize() override;
	
	void OnObjectTransacted(UObject* Object, const class FTransactionObjectEvent& TransactionEvent);
	
	void OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap);
	
	void OnMapOpened(const FString& String, bool bArg);
	
	void OnObjectRenamed(UObject* RenamedObject, UObject* RenamedObjectOuter, FName OldName) const;

	static TSharedPtr<IContentBrowserHideFolderIfEmptyFilter> Filter;	
	
	void OnLevelScriptContainerCreated(IBangoScriptHolderInterface& ScriptHolder, FString BlueprintName = "");

	void OnLevelScriptContainerDestroyed(IBangoScriptHolderInterface& ScriptHolder);

	void OnLevelScriptContainerDuplicated(IBangoScriptHolderInterface& ScriptHolder);
	
	// ------------------------------------------
	// Level script creation functions
private:
	void EnqueueCreatedScriptContainer(IBangoScriptHolderInterface& ScriptHolder);
	
	void EnqueueDestroyedScriptContainer(IBangoScriptHolderInterface& ScriptHolder);

	void RequestScriptQueueProcessing();
	
	// Master queue processing function
	void ProcessScriptRequestQueues();
	
	// Script creation methods
	void ProcessCreatedScriptRequest(TWeakObjectPtr<UObject> Owner, FBangoScriptContainer* ScriptContainer);
	
	void CreateLevelScript(IBangoScriptHolderInterface& ScriptHolder);
	
	void DuplicateLevelScript(IBangoScriptHolderInterface& ScriptHolder);
	
	void TryUndeleteScript(FSoftObjectPath ScriptClassSoft, FBangoScriptContainer* ScriptContainer);
	
	// Script destruction method
	void ProcessDestroyedScriptRequest(TSoftClassPtr<UBangoScript> ScriptClass);
	
	// -----------------------------------------
	// Level asset duplication/deletion handling
	void OnInitialAssetRegistrySearchComplete();
	
	void OnAssetAdded(const FAssetData& AssetData);
	
	void OnAssetRemoved(const FAssetData& AssetData);
	
	void OnAssetPostImport(UFactory* Factory, UObject* Object);
	
	void OnAssetPreDelete(UFactory* Factory, UObject* Object);
	
	void OnAssetsPreDelete(const TArray<UObject*>& Objects);
	
	void OnAssetsDeleted(const TArray<UClass*>& Classes);
	
	void OnRequestScriptSave(TSoftClassPtr<UBangoScript> Class);
	
private:
	TSet<FScriptContainerKey> CreationRequests;
	
	TSet<TSoftClassPtr<UBangoScript>> DestructionRequests;
	
	FTimerHandle ProcessScriptRequestQueuesHandle;
	
	// When duplicating levels, we are going to duplicate and force-save script packages (similar to what OFPA does when duplicating levels)
	bool bScriptProcessingLevelDuplication = false;	
};
