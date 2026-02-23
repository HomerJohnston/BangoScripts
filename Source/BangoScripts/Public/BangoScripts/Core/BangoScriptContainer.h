#pragma once

#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "Engine/StreamableManager.h"
#include "StructUtils/PropertyBag.h"

#include "BangoScriptContainer.generated.h"

class UBangoScript;
class UBangoScriptBlueprint;

/**
 * This struct is used to hold a script. It is used in ABangoTrigger and in UBangoScriptComponent.
 */
USTRUCT()
struct BANGOSCRIPTS_API FBangoScriptContainer
{
	GENERATED_BODY()
	
public:
	FBangoScriptContainer();
	
	friend class FBangoScriptContainerCustomization;
	
public:
	//FBangoScriptHandle Run(UObject* Runner, bool bImmediate);
	
	const TSoftClassPtr<UBangoScript>& GetScriptClass() const { return ScriptClass; }

	const FInstancedPropertyBag* GetPropertyBag() const { return &ScriptInputs; }
	
private:
	/** A brief description of the blueprint. This can be displayed in the level editor viewport. */
	UPROPERTY(EditInstanceOnly)
	FString Description = "";
	
	UPROPERTY(VisibleAnywhere)
	TSoftClassPtr<UBangoScript> ScriptClass = nullptr;
	
	// Used for async loading
	TSharedPtr<FStreamableHandle> ScriptClassHandle = nullptr;
	
	UPROPERTY(EditAnywhere)
	FInstancedPropertyBag ScriptInputs;
	
public:
#if WITH_EDITORONLY_DATA
	/** This is not used in any active code. It is only used to determine if an actor has already been set to a specific reference type in the script graph. */
	UPROPERTY(VisibleAnywhere)
	TSet<TSoftObjectPtr<AActor>> SoftActorRefs;
#endif
	
	/** This is not actively used for anything. Storing these forces World Partition to link this script owner actor with these actors so they're loaded together. This is required to remain in shipping runtime. */
	UPROPERTY(VisibleAnywhere)
	TSet<TObjectPtr<AActor>> HardActorRefs;
	
	// TODO: ACTOR STREAMABLE REFS
	// This is disabled due to a plethora of technical challenges. I may try to come up with something to make it work one day.
	// The intent was to make it possible for a script running to spawn a streaming source at the target actor, but it's VERY difficult to do this reliably.
	/** Actors stored in here will get streaming sources spawned on them while the script runs. Location is ONLY updated during cook. */
	// UPROPERTY(VisibleAnywhere)
	// TMap<TSoftObjectPtr<AActor>, FVector> StreamingSourceActorRefs;
	
protected:
	TArray<TWeakObjectPtr<AActor>> SpawnedStreamingSources;
	
public:
	void SpawnStreamingSources(UObject* WorldContext);
	
	void DespawnStreamingSources();
	
#if WITH_EDITORONLY_DATA
	// Used during construction of the ScriptClass only
	FString RequestedName = "";
	bool bNewLeveScriptRequested = false;
	bool bCreated = false;
	
	UPROPERTY(Transient)
	bool bIsDuplicate = false;
#endif
	
#if WITH_EDITORONLY_DATA
private:
	// This will be kept in sync with the UBangoScriptObject's ScriptGuid and is used for undo/redo purposes and other sync
	UPROPERTY(EditAnywhere)
	FGuid Guid;
	
public:
	const FGuid& GetGuid() const { return Guid; }
	
	void SetGuid(const FGuid& InGuid);
#endif
	
public:
#if WITH_EDITOR
	void Unset();
	
	void SetScriptClass(TSubclassOf<UObject> NewScriptClass);
	
	void UpdateScriptInputs();
	
	void GetPropertiesForRefresh(TArray<FProperty*>& NonExistentProperties, TArray<FName>& DeadProperties) const;
	
	void SetRequestedName(const FString& InName);

	const FString& GetRequestedName() const;

	void SetNewLevelScriptRequested();
	
	void MarkForNewLevelScript();
	
	// void MarkDuplicated();

	bool ConsumeNewLevelScriptRequest();
	
	bool ConsumeMarkForNewLevelScript();
	
	// bool ConsumeMarkDuplicated();

	// bool IsMarkedDuplicated() const { return bIsDuplicate; };
	
	const FString& GetDescription() const;

	bool AreScriptInputsOutDated() const;

	uint64 CachedFrameCheck = 0;
	bool CachedResult = false;

#endif

};
