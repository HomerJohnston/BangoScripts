#pragma once

#include "BangoScripts/Core/BangoScriptContainer.h"
#include "InputCoreTypes.h"
#include "Components/ActorComponent.h"
#include "BangoScripts/Debug/BangoDebugDrawServiceBase.h"

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
class BANGOSCRIPTS_API UBangoScriptComponent : public UActorComponent
{
	GENERATED_BODY()
	
	friend class UBangoLevelScriptsEditorSubsystem;
	friend class UBangoScriptBlueprint;
	
public:
	UBangoScriptComponent();
	
	void BeginPlay() override;
	
#if WITH_EDITOR
public:
	void OnRegister() override;
	
	void OnUnregister() override;
	
	// This is only used to spawn script assets for level instance added components
	void OnComponentCreated() override;

	void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	
	void BeginDestroy() override;
	
	void FinishDestroy() override;
	
	// This is only used to spawn script assets for CDO spawned components (actor dragged into world) as well as for duplicating any actors or any instance components
	void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	
	void UnsetScript();

	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	//void OnRename();
#endif
	
protected:
	/** Use this to run the script automatically upon BeginPlay. */
	UPROPERTY(EditAnywhere, DisplayName = "Autoplay")
	bool bRunOnBeginPlay = false;
	
	/** The actual script instance. */
	UPROPERTY(EditInstanceOnly)
	FBangoScriptContainer ScriptContainer;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	FBangoScriptHandle RunningHandle;
	
	// I am toying with using a standard billboard component to represent the script instead of debugdraw, not sure yet.
    UPROPERTY(Transient)
    TObjectPtr<UBillboardComponent> Billboard;
    
    UPROPERTY(EditAnywhere)
    bool bUseDebugDraw = false;
#endif
	
public:
	/** Runs the script. */
	UFUNCTION(BlueprintCallable)
	void Run();
	
#if WITH_EDITOR
public:
	
	FGuid GetScriptGuid() const;
	
	UBangoScriptBlueprint* GetScriptBlueprint(bool bForceLoad = false) const;
	
	void SetScriptBlueprint(UBangoScriptBlueprint* Blueprint); 
	
	void OnScriptFinished(FBangoScriptHandle FinishedHandle);
	
	void PerformDebugDraw(FBangoDebugDrawCanvas& Canvas, bool bPIE);
	
	static TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)> OnDebugDrawEditor;
	
	static TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent)> OnDebugDrawPIE;
	
	void PreEditUndo() override;

	void PostEditUndo(TSharedPtr<ITransactionObjectAnnotation> TransactionAnnotation) override;
	
	UBillboardComponent* GetBillboard() const { return Billboard; }
	
	const FBangoScriptHandle& GetRunningHandle() const { return RunningHandle; }
	
	bool GetRunOnBeginPlay() const { return bRunOnBeginPlay; }
	
	const FBangoScriptContainer& GetScriptContainer() const { return ScriptContainer; }
#endif
};