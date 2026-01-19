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
	
#if WITH_EDITORONLY_DATA
	// Used during construction of the ScriptClass only
	FString RequestedName = "";
	bool bNewLeveScriptRequested = false;
	bool bIsDuplicate = false;
#endif
	
#if WITH_EDITORONLY_DATA
private:
	// This will be kept in sync with the UBangoScriptObject's ScriptGuid and is used for undo/redo purposes and other sync
	UPROPERTY()
	FGuid Guid;
	
public:
	const FGuid& GetGuid() const { return Guid; }
	
	void SetGuid(const FGuid& InGuid);
#endif
	
public:
#if WITH_EDITOR
	void Unset();
	
	void SetScriptClass(TSubclassOf<UObject> NewScriptClass);
	
	bool AreScriptInputsOutDated();
	
	void UpdateScriptInputs();
	
	void GetPropertiesForRefresh(TArray<FProperty*>& NonExistentProperties, TArray<FName>& DeadProperties) const;
	
	void SetRequestedName(const FString& InName);

	const FString& GetRequestedName() const;

	void SetIsDuplicate();

	bool ConsumeNewLevelScriptRequest();
	
	bool ConsumeDuplicate();

	const FString& GetDescription() const;
#endif

};
