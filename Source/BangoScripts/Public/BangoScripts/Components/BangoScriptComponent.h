#pragma once

#include "BangoScripts/Core/BangoScriptContainer.h"
#include "InputCoreTypes.h"
#include "Components/ActorComponent.h"
#include "BangoScripts/Debug/BangoDebugDrawServiceBase.h"
#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"
#include "Math/GenericOctreePublic.h"

#include "BangoScriptComponent.generated.h"

class UBlueprint;
class UBangoScript;

/** Control when or where this script is allowed to run. */
UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class EBangoScriptComponentAllowedRunContexts : uint8
{
	None		= 0			UMETA(Hidden),
	Editor		= 1 << 0	UMETA(ToolTip = "This script will only run in Editor (PIE/SIE)"),
	Standalone	= 1 << 1	UMETA(ToolTip = "This script will only run in Standalone")
};

ENUM_CLASS_FLAGS(EBangoScriptComponentAllowedRunContexts)

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class EBangoScriptComponentAllowedBuildConfigs : uint8
{
	None			= 0 UMETA(Hidden),
	Development		= 1 << 1, // This script is allowed to run in normal development and debug builds
	Test			= 1 << 2, // This script is allowed to run in UE_BUILD_TEST builds
	Shipping		= 1 << 3, // This script is allowed to run in UE_BUILD_SHIPPING builds
};

ENUM_CLASS_FLAGS(EBangoScriptComponentAllowedBuildConfigs)

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class EBangoScriptComponentAllowedNetConfigs : uint8
{
	None			= 0 UMETA(Hidden),
	Server			= 1 << 0, // This script is allowed to run on hosts
	Client			= 1 << 1, // This script is allowed to run on clients
};

ENUM_CLASS_FLAGS(EBangoScriptComponentAllowedNetConfigs)

USTRUCT()
struct FBangoScriptComponent_BillboardSettings
{
	GENERATED_BODY()
	
	/** Disables display of the billboard entirely. Has no effect on other things (use viewport Developer showflag "Bango Scripts" to hide other debug elements).*/
	UPROPERTY(Category = "Bango", EditAnywhere)
	bool bDisable = false;
	
	/** Moves the billboard representer around if needed. */
	UPROPERTY(Category = "Bango", EditAnywhere)
	FVector BillboardOffset = FVector(0.0f, 0.0f, 100.0f);
	
	/** If set, overrides the default "scroll" billboard sprite. */
	UPROPERTY(Category = "Bango", EditAnywhere)
	TSoftObjectPtr<UTexture2D> CustomBillboard = nullptr; 
};

UCLASS(meta = (BlueprintSpawnableComponent), HideCategories = ("Activation", "AssetUserData", "Cooking", "Navigation", "Tags", "ComponentTick", "Sockets", "ComponentReplication", "Replication"))
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
	
	void PostLoad() override;
	
	void BeginDestroy() override;
	
	void FinishDestroy() override;
	
	// This is only used to spawn script assets for CDO spawned components (actor dragged into world) as well as for duplicating any actors or any instance components
	void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

	void PostEditImport() override;
	
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly) override;
	
	void PreSave(FObjectPreSaveContext SaveContext) override;
	
	bool bSaving = false;
	
	void PreSaveRoot(FObjectPreSaveRootContext ObjectSaveContext) override;
	
	void PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext) override;
	
	//void OnRename();
#endif
	
protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = "Bango", DisplayName = "Billboard", EditAnywhere)
	FBangoScriptComponent_BillboardSettings BillboardSettings;
	
	/** If set, will apply a comment onto the start node when a script is created. */
	UPROPERTY(Category = "Bango", EditDefaultsOnly)
	FString StartNodeComment;
#endif
	
	/** The actual script instance. */
	UPROPERTY(Category = "Bango", EditInstanceOnly)
	FBangoScriptContainer ScriptContainer;
	
	/** Use this to run the script automatically upon BeginPlay. */
	UPROPERTY(Category = "Bango", EditAnywhere, DisplayName = "Autoplay")
	bool bRunOnBeginPlay = false;
	
	// TODO should this be editor-only? Can I reset this upon editor restart?
	/** If set, this script will never be ran. Intended for debugging purposes. Has no effect in Standalone! */
	UPROPERTY(Category = "Bango", EditInstanceOnly, DisplayName = "Suppress (Editor-Only)")
	bool bPreventExecution = false;
	
	/** Control where this script is allowed to run. */
	UPROPERTY(Category = "Bango", AdvancedDisplay, EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/BangoScripts.EBangoScriptComponentAllowedRunContexts"))
	uint8 AllowedRunContexts;
	
	/** Control where this script is allowed to run. */
	UPROPERTY(Category = "Bango", AdvancedDisplay, EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/BangoScripts.EBangoScriptComponentAllowedBuildConfigs"))
	uint8 AllowedBuildConfigs;
	
	/** Control where this script is allowed to run. */
	UPROPERTY(Category = "Bango", AdvancedDisplay, EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/BangoScripts.EBangoScriptComponentAllowedNetConfigs"))
	uint8 AllowedNetConfigs;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	FBangoScriptHandle RunningHandle;

    UPROPERTY(Transient)
    TObjectPtr<UBillboardComponent> BillboardInstance;
	
public:
	FOctreeElementId2 DebugElementId;
	
protected:
	bool bDebugRegistered = false;
#endif
	
public:
	/** Runs the script. */
	UFUNCTION(BlueprintCallable)
	void Run();
	
protected:
	bool ContextPassesAllowFlags() const;
	
#if WITH_EDITOR
public:
	
	UBangoScriptBlueprint* GetScriptBlueprint(bool bForceLoad = false) const;
	
	void SetScriptBlueprint(UBangoScriptBlueprint* Blueprint); 
	
	void OnScriptFinished(FBangoScriptHandle FinishedHandle);
	
	static TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent)> OnDebugDrawEditor;
	
	// This one is non-const because during PIE/Simulate we have the option of dynamically running the script. UBangoScriptComponent::Run() is mutable.
	static TMulticastDelegate<void(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent)> OnDebugDrawPIE;
	
	void PreEditUndo() override;

	void PostEditUndo(TSharedPtr<ITransactionObjectAnnotation> TransactionAnnotation) override;
	
	UBillboardComponent* GetBillboard() const { return BillboardInstance; }
	
	const FBangoScriptHandle& GetRunningHandle() const { return RunningHandle; }
	
	bool GetRunOnBeginPlay() const { return bRunOnBeginPlay; }
	
	bool GetPreventExecution() const { return bPreventExecution; }
	
	FBangoScriptContainer& GetScriptContainer() override { return ScriptContainer; }
	
	const FBangoScriptContainer& GetScriptContainer() const { return ScriptContainer; }
	
	FVector GetDebugDrawOrigin() const override;

	FVector GetBillboardPosition() const;
	
	const FVector& GetBillboardOffset() const { return BillboardSettings.BillboardOffset; }
	
	const bool IsBillboardEnabled() const { return !BillboardSettings.bDisable; }
	
	IBangoScriptHolderInterface& AsScriptHolder() { return *Cast<IBangoScriptHolderInterface>(this); }
	
	const FString& GetStartEventComment() const override { return StartNodeComment; }
	
	bool HasValidScript() const;
	
	void LogStatus(FString* OutString = nullptr) const override;
	
protected:
	void UpdateBillboard() override;
#endif
	
	
};