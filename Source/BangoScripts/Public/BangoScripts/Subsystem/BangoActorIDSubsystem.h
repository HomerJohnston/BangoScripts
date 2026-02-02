#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/WorldSubsystem.h"

#include "BangoActorIDSubsystem.generated.h"

struct FBangoScriptHandle;
class UBangoScript;

using FGuidRegistration = TPair<TWeakObjectPtr<AActor>, FGuid>;
using FNameRegistration = TPair<TWeakObjectPtr<AActor>, FName>;

UCLASS()
class UBangoActorIDSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	static UBangoActorIDSubsystem* Get(UObject* WorldContext);

	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	
protected:
	TMap<FName, FGuidRegistration> ActorsByName;
	
	TMap<FGuid, FNameRegistration> ActorsByGuid;
	
public:
	static void RegisterActor(UObject* WorldContextObject, AActor* Actor, FName Name, FGuid Guid);

	static void UnregisterActor(UObject* WorldContextObject, FGuid Guid);

	static void UnregisterActor(UObject* WorldContextObject, FName Name);
	
	static AActor* GetActor(UObject* WorldContextObject, FName Name);
	
	static AActor* GetActor(UObject* WorldContextObject, FGuid Guid);
};

UCLASS()
class UBangoActorIDBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintInternalUseOnly, BlueprintCallable, BlueprintPure, Category = "Bango", DisplayName = "Get Actor by Name", meta = (WorldContext = "WorldContextObject"))
	static AActor* K2_GetActorByName(UObject* WorldContextObject, FName Name);
	
	UFUNCTION(BlueprintInternalUseOnly,BlueprintCallable, BlueprintPure, Category = "Bango", DisplayName = "Get Actor by GUID", meta = (WorldContext = "WorldContextObject"))
	static AActor* K2_GetActorByGuid(UObject* WorldContextObject, FGuid Guid);
};