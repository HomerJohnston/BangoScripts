#pragma once

#include "BangoScripts/Components/BangoScriptComponent.h"

#include "BangoLevelScriptsEditorSubsystem.generated.h"

class UObject;
class UWorld;

using namespace Bango;

// ==============================================

namespace Bango
{
	struct FScriptContainerKey
	{
		FScriptContainerKey(IBangoScriptHolderInterface& ScriptHolder)
		{
			UObject* OuterObject = ScriptHolder._getUObject();
			
			if (OuterObject->GetPackage() == GetTransientPackage())
			{
				OuterPtr = OuterObject;
			}
			else
			{
				Outer = OuterObject;
			}
			
			ScriptContainer = &ScriptHolder.GetScriptContainer();
			Script = ScriptContainer->GetScriptClass();
		}
		
		// ----------------------------
		
	protected:
		// Most of the time we'll use a soft object path, but I've found copy-pasting and relying on PostEditImport requires usage of actual pointer address
		// since the component is often spawned into the transient package and then later moved.
		TOptional<FSoftObjectPath> Outer;
		TOptional<UObject*> OuterPtr;
		
	public:
		FBangoScriptContainer* ScriptContainer = nullptr;
		TSoftClassPtr<UBangoScript> Script;

		// ----------------------------
		
		bool operator==(const FScriptContainerKey& Other) const
		{
			return GetObject() == Other.GetObject();
		}

		// ----------------------------
		
		UObject* GetObject() const
		{
			if (OuterPtr.IsSet())
			{
				return OuterPtr.GetValue();
			}
			
			return Outer->ResolveObject();
		}
		
		friend int32 GetTypeHash(const FScriptContainerKey& ScriptContainerKey)
		{
			// This limits me to one script per UObject... I don't know if I can get it working any other way
			return HashCombine(GetTypeHash(ScriptContainerKey.GetObject())); // , GetTypeHash(ScriptContainerKey.ScriptContainer->GetGuid()));
			
			/*
			if (ScriptContainerKey.ScriptContainer->GetScriptClass().IsNull())
			{
				return HashCombine(GetTypeHash(ScriptContainerKey.Outer), GetTypeHash(ScriptContainerKey.ScriptContainer));
			}

			return GetTypeHash(ScriptContainerKey.ScriptContainer->GetScriptClass().ToString());
			*/
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
	bool bSavingLevel = false;
	
	FDelegateHandle OnAssetAddedHandle;
	FDelegateHandle OnAssetRemovedHandle;

	TSet<FSoftObjectPath> NewObjects;
	TSet<FScriptContainerKey> DuplicatingObjects;
	TSet<FSoftObjectPath> DeletedScripts;
	
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void Deinitialize() override;
	
	void OnObjectTransacted(UObject* Object, const class FTransactionObjectEvent& TransactionEvent);
	
	void OnMapLoad(const FString& String, FCanLoadMap& CanLoadMap);
	
	void OnMapOpened(const FString& String, bool bArg);
	
	void OnObjectRenamed(UObject* RenamedObject, UObject* RenamedObjectOuter, FName OldName) const;
	
	void OnObjectConstructed(UObject* Object);

	static TSharedPtr<IContentBrowserHideFolderIfEmptyFilter> Filter;	
	
	void OnLevelScriptContainerCreated(IBangoScriptHolderInterface& ScriptHolder, FString BlueprintName = "");

	void OnLevelScriptContainerDestroyed(IBangoScriptHolderInterface& ScriptHolder, EBangoScriptDeletedHelper Flag);

	void OnLevelScriptContainerDuplicated(IBangoScriptHolderInterface& ScriptHolder);
	
	// ------------------------------------------
	// Level script creation functions
private:
	void EnqueueChangedScriptContainer(const Bango::FScriptContainerKey Key);
	
	void RequestScriptQueueProcessing();
	
	// Master queue processing function
	void ProcessScriptRequestQueues();
	
	// Script creation methods
	void ProcessCreatedScriptRequest(UObject& Owner, const Bango::FScriptContainerKey& CreationRequest);
	
	void CreateLevelScript(IBangoScriptHolderInterface& ScriptHolder);
	
	void DuplicateLevelScript(IBangoScriptHolderInterface& ScriptHolder, TSoftClassPtr<UBangoScript> ScriptClass);
	
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
	
	void OnRequestScriptSave(IBangoScriptHolderInterface* ScriptHolder, TSoftClassPtr<UBangoScript> Class);
	
private:
	TSet<FScriptContainerKey> ChangeRequests;
	
	FTimerHandle ProcessScriptRequestQueuesHandle;
	
	// When duplicating levels, we are going to duplicate and force-save script packages (similar to what OFPA does when duplicating levels)
	bool bScriptProcessingLevelDuplication = false;	
	
	bool CanProcessScripts() const;
};
