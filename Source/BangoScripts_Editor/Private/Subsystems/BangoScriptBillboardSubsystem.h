#pragma once

#include "TickableEditorObject.h"
#include "BillboardGenerator/BangoAsyncBillboardGenerator.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "UObject/ObjectKey.h"

#include "BangoScriptBillboardSubsystem.generated.h"

class IBangoScriptHolderInterface;

struct FBangoBillboardRequest
{
	TSet<FObjectKey> RequestingObjects; // IBangoScriptHolderInterface implementers
	TSharedPtr<FBangoAsyncBillboardGenerator> Generator = nullptr;
};

UCLASS()
class UBangoScriptBillboardSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

protected:
	TMap<TSoftObjectPtr<UTexture2D>, FBangoBillboardRequest> QueuedRequests;

	TSharedPtr<FBangoAsyncBillboardGenerator> ActiveGenerator;

	FTimerHandle TickTimerHandle;
	
	void OnCustomBillboardRequested(IBangoScriptHolderInterface* Requester, const TSoftObjectPtr<UTexture2D>& OverlayTexture);
	
	void Tick();
	
	void OnGenerationComplete(TSharedPtr<FBangoAsyncBillboardGenerator> Generator);
	
protected:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void Deinitialize() override;
};