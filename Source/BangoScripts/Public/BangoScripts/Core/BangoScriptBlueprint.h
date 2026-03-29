#pragma once

#include "BangoScriptObjectPath.h"
#include "BangoScripts/Core/BangoScript.h"
#include "Engine/Blueprint.h"
#include "IO/PackageId.h"

#include "BangoScriptBlueprint.generated.h"

class IBangoScriptHolderInterface;

UCLASS()
class BANGOSCRIPTS_API UBangoScriptBlueprint : public UBlueprint
{
	GENERATED_BODY()

public:
	UBangoScriptBlueprint();
	
#if WITH_EDITOR

	// ------------------------------------------
	// UBlueprint overrides

public:
	bool ShouldBeMarkedDirtyUponTransaction() const override;
	
	// ------------------------------------------
	// Common editor functionality

public:
	const TSoftObjectPtr<AActor> GetOwnerActor() const;
	
	const IBangoScriptHolderInterface* GetScriptHolder() const;
	
	IBangoScriptHolderInterface* GetScriptHolderMutable();
	
	void SetOwnerActor(AActor* Actor);
	
	void SetScriptHolder(IBangoScriptHolderInterface& ScriptHolder);
	
	const FGuid& GetScriptGuid();

	void SetScriptGuid(FGuid InGuid);

	// Used by blueprint duplication
	void Reset();
	
	TWeakObjectPtr<UObject> GetCurrentObjectBeingDebugged() const;

	// ------------------------------------------
	// Delete/Undo support
	// These are assigned when a script is deleted (i.e. when something owning a script is deleted), and is used to restore the script back to an identical-to-original state.
	
public:
	// The original name, e.g. ~BangoScript
	FString DeletedName;
	
	// The original package path, e.g. /Game/__BangoScripts__/Level/ActorID/ScriptID/BangoScript.uasset
	FSoftObjectPath DeletedPackagePath; 
	
	// UPackage Guid
	FGuid DeletedPackagePersistentGuid; 
	
	// UPackage ID
	FPackageId DeletedPackageId; 

	// ------------------------------------------
	// Static global helpers
	
public:
	static UBangoScriptBlueprint* GetBangoScriptBlueprintFromClass(const TSoftClassPtr<UBangoScript> ScriptClass);
	
#endif
	
#if WITH_EDITORONLY_DATA
	
	// ------------------------------------------
	// Script settings / data members
protected:
    
	/* 
	Every script will have a unique ID. This is used to uniquely rename and rediscover scripts when they are deleted and undo'd. 
	Upon deleting a script container, the script asset is renamed into the transient package with its Guid as its unique name.
	Upon undoing a deletion, the script is discovered by its Guid name and is restored to its previous name. 
	*/
	UPROPERTY(VisibleAnywhere, NonPIEDuplicateTransient, TextExportTransient)
	FGuid ScriptGuid;
	
	/* 
	We use a raw FString to avoid packaging/asset reference discovery issues with UE thinking that the actor is referenced by this script. 
	This is only used for blueprint editor niceties and has no effect on gameplay.
	Format is a standard object path, e.g.: 
	/Game/FolderPath/LevelName.LevelName:PersistentLevel.Type_C_# 
	*/
    UPROPERTY(EditAnywhere, NonPIEDuplicateTransient, TextExportTransient)
    FBangoScriptObjectPath OwnerActorPath;
    
	/* 
	This is the path to the actual UObject that implements IBangoScriptHolderInterface. This is usually a Bango Script Component of the owner actor, e.g.:
	/Game/FolderPath/LevelName.LevelName:PersistentLevel.Type_C_17.BangoScriptComponentName 
	*/
	UPROPERTY(EditAnywhere, NonPIEDuplicateTransient, TextExportTransient)
	FBangoScriptObjectPath ScriptHolderObjectPath;
#endif
};
