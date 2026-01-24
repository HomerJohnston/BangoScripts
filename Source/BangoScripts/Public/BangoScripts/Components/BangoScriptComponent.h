#pragma once

#include "BangoScripts/Core/BangoScriptContainer.h"
#include "InputCoreTypes.h"
#include "Components/ActorComponent.h"
#include "BangoScripts/Debug/BangoDebugDrawServiceBase.h"
#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"

#include "BangoScriptComponent.generated.h"

class UBlueprint;
class UBangoScript;

// TODO think more whether I want this. I currently just set 'This' to the closest owner actor.
/*
UENUM(BlueprintType)
enum class EBangoScriptComponent_ThisArg : uint8
{
	OwnerActor,
	ScriptComponent,
};
*/

UCLASS(meta = (BlueprintSpawnableComponent), HideCategories = ("Activation", "AssetUserData", "Cooking", "Navigation", "Tags"))
class BANGOSCRIPTS_API UBangoScriptComponent : public UActorComponent, public IBangoScriptHolderInterface
{
	GENERATED_BODY()
	
public:
	UBangoScriptComponent();
	
	void BeginPlay() override;
	
#if WITH_EDITOR
public:
	UFUNCTION()
	void OnRegister() override;
	
	void OnUnregister() override;
	
	// This is only used to spawn script assets for level instance added components
	void OnComponentCreated() override;

	void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	
	void PostApplyToComponent() override;
	
	void BeginDestroy() override;
	
	void FinishDestroy() override;
	
	// This is only used to spawn script assets for CDO spawned components (actor dragged into world) as well as for duplicating any actors or any instance components
	void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	//void OnRename();
#endif
	
protected:
#if WITH_EDITORONLY_DATA
	/** Moves the billboard representer around if needed. */
	UPROPERTY(Category = "Bango", EditAnywhere)
	FVector BillboardOffset;
#endif
	
	/** The actual script instance. */
	UPROPERTY(Category = "Bango", EditInstanceOnly)
	FBangoScriptContainer ScriptContainer;
	
	/** Use this to run the script automatically upon BeginPlay. */
	UPROPERTY(Category = "Bango", EditAnywhere, DisplayName = "Autoplay")
	bool bRunOnBeginPlay = false;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	FBangoScriptHandle RunningHandle;
	
	// I am toying with using a standard billboard component to represent the script instead of debugdraw, not sure yet.
    UPROPERTY(Transient)
    TObjectPtr<UBillboardComponent> Billboard;
#endif
	
public:
	/** Runs the script. */
	UFUNCTION(BlueprintCallable)
	void Run();
	
#if WITH_EDITOR
public:
	
	UBangoScriptBlueprint* GetScriptBlueprint(bool bForceLoad = false) const;
	
	void SetScriptBlueprint(UBangoScriptBlueprint* Blueprint); 
	
	void OnScriptFinished(FBangoScriptHandle FinishedHandle);
	
	void PerformDebugDrawUpdate(FBangoDebugDrawCanvas& Canvas, bool bPIE);
	
	static TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)> OnDebugDrawEditor;
	
	static TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)> OnDebugDrawPIE;
	
	void PreEditUndo() override;

	void PostEditUndo(TSharedPtr<ITransactionObjectAnnotation> TransactionAnnotation) override;
	
	UBillboardComponent* GetBillboard() const { return Billboard; }
	
	const FBangoScriptHandle& GetRunningHandle() const { return RunningHandle; }
	
	bool GetRunOnBeginPlay() const { return bRunOnBeginPlay; }
	
	FBangoScriptContainer& GetScriptContainer() override { return ScriptContainer; }
	
	const FBangoScriptContainer& GetScriptContainer() const { return ScriptContainer; }
	
	const FVector& GetDebugDrawOrigin() const override { return GetBillboardOffset(); }
	
	const FVector& GetBillboardOffset() const { return BillboardOffset; }
	
	IBangoScriptHolderInterface& AsScriptHolder() { return *Cast<IBangoScriptHolderInterface>(this); }
	
protected:
	void UpdateBillboard();
#endif
	
	
};