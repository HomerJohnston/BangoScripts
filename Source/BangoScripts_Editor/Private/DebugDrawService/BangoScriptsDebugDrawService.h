

#pragma once

#include "EditorSubsystem.h"
#include "TickableEditorObject.h"
#include "Components/ActorComponent.h"
#include "Engine/Canvas.h"
#include "Math/GenericOctree.h"

#include "BangoScriptsDebugDrawService.generated.h"

class AActor;
class APlayerController;
class FBangoScriptBlueprintEditor;
class IMenu;
class UBangoScriptComponent;
class UCanvas;
class USceneComponent;
struct FBangoDebugDrawCanvas;
enum class EBangoScriptComponentRegisterStatus : uint8;

// ================================================================================================

struct FBangoScriptOctreeElement
{
	FBangoScriptOctreeElement(UBangoScriptComponent* InScriptComponent);
	
	FVector Position;
	
	TWeakObjectPtr<UBangoScriptComponent> ScriptComponent;
	
	// FOctreeElementId2 ElementId;
	
	// Returns true if position changed
	bool Update();
};

// ================================================================================================

struct FBangoScriptOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const FBangoScriptOctreeElement& Element)
	{
		return FBoxCenterAndExtent(Element.Position, FVector::ZeroVector);
	}

	FORCEINLINE static bool AreElementsEqual(const FBangoScriptOctreeElement& A, const FBangoScriptOctreeElement& B)
	{
		return (A.ScriptComponent == B.ScriptComponent);
	}

	static void SetElementId(const FBangoScriptOctreeElement& Element, FOctreeElementId2 Id);
};

// ================================================================================================

struct FBangoDebugDraw_ScriptComponentHover
{
	// Try to set new focus billboard. Returns if a new contender was selected.
	bool TryToFocusOnComponent(const UBangoScriptComponent* Contender, float MouseDistanceToBillboard);

	// Getter
	TWeakObjectPtr<const UBangoScriptComponent> GetFocusedComponent() const;

	// Sets the two TSharedPtr vars
	void SetActiveMenuAndWidget(TSharedPtr<SWidget> InMenuWidget, TSharedPtr<IMenu> InMenu);
	
	// Unset everything
	void Reset();
	
	// Do we have a focused component, but hover has not been consumed yet?
	bool IsHoveredOrHoverPendingFor(UBangoScriptComponent* ScriptComponent) const;
	
	// Is there currently an active widget?
	bool HasFocusedComponent() const;
	
	// Is the component currently hovered?
	bool IsWidgetVisibleAndHovered() const;
	
	bool ConsumeHovered();
	
private:
	void SwitchFocus(const UBangoScriptComponent* NewFocus);
	
	TWeakObjectPtr<const UBangoScriptComponent> FocusedComponent = nullptr;
	
	float ScreenDistance = -1.0f;	
	TSharedPtr<SWidget> ActiveMenuWidget;
	TSharedPtr<IMenu> ActiveMenu;
	bool bSlateThrottle = false;
	float StartFocusTime = -1.0f;

	bool bHoverConsumed = false;
};

// ================================================================================================

struct FBangoScripts_NearbyScript
{
	FBangoScripts_NearbyScript(UBangoScriptComponent* InComponent, float InMouseDistSqrd, FVector2f InScreenPos) 
		: Component(InComponent)
		, MouseDistSqrd(InMouseDistSqrd)
		, ScreenPos(InScreenPos){}
	
	UBangoScriptComponent* Component;
	float MouseDistSqrd;
	FVector2f ScreenPos;
};

UCLASS()
class UBangoScriptsDebugDrawService : public UEditorSubsystem, public FTickableEditorObject
{
	GENERATED_BODY()

	bool bShowFlagEnabled;

	uint64 DebugDrawFrame;

	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void Deinitialize() override;
	
	bool IsTickable() const override;
	
	void DebugDraw(UCanvas* Canvas, APlayerController* ALWAYSNULL_DONOTUSE);
	
	void Tick(float DeltaTime) override;
	
	void UpdateNearbyScripts();
	
	TStatId GetStatId() const override;
	
	TOctree2<FBangoScriptOctreeElement, FBangoScriptOctreeSemantics> ScriptComponentTree;
	
	// TODO | This is a hack. An Actor's root component has a TransformUpdated delegate which should be used to detect if the actor is moved, but dragging the actor around does not fire this delegate.
	// TODO | So instead we sub to GEngine->OnActorMoved and check EVERY ACTOR to see if they are involved in this debug draw. We store our own actors in this set to speed this up a bit.
	UPROPERTY(Transient)
	TSet<AActor*> ScriptOwners;

	FDelegateHandle DebugDrawHandle;
	
private:
	FBangoDebugDraw_ScriptComponentHover HoverInfo;
	
	TArray<FBangoScripts_NearbyScript> NearbyScripts;
	
private:
	void OnBangoScriptRegistrationChange(UBangoScriptComponent* ScriptComponent, EBangoScriptComponentRegisterStatus RegistrationStatus);
	
	void OnScriptComponentMoved(USceneComponent* SceneComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport, TWeakObjectPtr<UBangoScriptComponent> ScriptComponent);
	
	void AddElement(FBangoScriptOctreeElement& Element);

	void RemoveElement(FBangoScriptOctreeElement& Element);
	
	void OnGlobalActorMoved(AActor* Actor);
	
	bool DrawViewportHoverControls(UBangoScriptComponent* ScriptComponent, float MouseDistSqrd, const FVector2f& BillboardScreenPos, bool bPIE);

	TSharedRef<SWidget> GetHoverMenuWidget(UBangoScriptComponent* ScriptComponent, bool bPIE);
	
	void DrawPIEIcon(UCanvas* Canvas, const UBangoScriptComponent* ScriptComponent, FVector2f ScreenPos);

	FText GetLabelText(const UBangoScriptComponent& ScriptComponent);
	
	TSharedPtr<SWidget> CreatePopupTitle(UBangoScriptComponent& ScriptComponent);
	TSharedPtr<SWidget> CreateEditButton(UBangoScriptComponent& ScriptComponent);
	TSharedPtr<SWidget> CreateNewButton(UBangoScriptComponent& ScriptComponent);
	TSharedPtr<SWidget> CreateRunButton(UBangoScriptComponent& ScriptComponent);
};

// ================================================================================================
