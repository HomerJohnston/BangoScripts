#pragma once

#if 0
#include "Components/BillboardComponent.h"
#include "Engine/World.h"

// #include "BangoDebugDraw_ScriptComponent.generated.h"

class AActor;
class APlayerController;
class UBangoScriptComponent;
class UCanvas;
class USceneComponent;
class FBangoScriptBlueprintEditor;
class IMenu;
struct FBangoDebugDrawCanvas;

struct FBangoDebugDraw_ScriptComponentHover
{
	TWeakObjectPtr<const UBangoScriptComponent> FocusedComponent = nullptr;
	float ScreenDistance;	
	float StartFocusTime;
    
    bool bSlateThrottle = false;
	
    //FVector CameraPos;
	//FVector CameraDir;
	
	TSharedPtr<IMenu> ActiveMenu;
	
	void Reset();

	// Try to set new focus billboard. Returns if a new contender was selected.
	bool Try(const UBangoScriptComponent* Contender, float MouseDistanceToBillboard);

	void SwitchFocus(const UBangoScriptComponent* NewFocus);
};

// TODO DEPRECATED, DELETE ME
// UCLASS(Abstract, NotBlueprintable)
class UBangoDebugDraw_ScriptComponent : public UObject
{
	GENERATED_BODY()
	
public:
	UBangoDebugDraw_ScriptComponent();

protected:
	static void DebugDrawPIE(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent);
	
	static void DebugDrawPIEImpl(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos);
	
	static void DrawViewportHoverControls(UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos, bool bPIE);
	
private:
	static FBangoDebugDraw_ScriptComponentHover HoverInfo;
	
	static FText GetLabelText(const UBangoScriptComponent& ScriptComponent);
};
#endif