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
	
protected:
	/** A brief description of the blueprint. This can be displayed in the level editor viewport. */
	UPROPERTY(EditInstanceOnly)
	FString Description = "";
	
	UPROPERTY(VisibleAnywhere)
	TSoftClassPtr<UBangoScript> ScriptClass = nullptr;
	
	// Used for async loading
	TSharedPtr<FStreamableHandle> ScriptClassHandle = nullptr;
	
	UPROPERTY(EditInstanceOnly)
	FInstancedPropertyBag ScriptInputs;
	
public:
#if WITH_EDITORONLY_DATA
	/** This is not used in any active code. It is only used to determine if an actor has already been set to a specific reference type in the script's graph. */
	UPROPERTY()
	TSet<TSoftObjectPtr<AActor>> SoftActorRefs;
#endif
	
	/** 
	 * YOU SHOULD NOT EDIT THIS. Storing actor refs here forces World Partition to link this script owner actor with these actors so they're loaded together. 
	 * This property will be hidden at a later date after this feature is stable. 
	 */
	UPROPERTY(EditInstanceOnly, AdvancedDisplay, DisplayName = "Hard Actor Refs (DEBUG VIEW)")
	TSet<TObjectPtr<AActor>> HardActorRefs;
	
#if WITH_EDITORONLY_DATA
protected:
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
	
	void ClearActorRefs();
	
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
