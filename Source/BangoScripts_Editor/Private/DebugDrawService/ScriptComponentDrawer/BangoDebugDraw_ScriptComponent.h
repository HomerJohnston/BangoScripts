#pragma once
#include "Components/BillboardComponent.h"
#include "Engine/World.h"

#include "BangoDebugDraw_ScriptComponent.generated.h"

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

	//FVector CameraPos;
	//FVector CameraDir;
	
	TSharedPtr<IMenu> ActiveMenu;
	
	void Reset();

	// Try to set new focus billboard. Returns if a new contender was selected.
	bool Try(const UBangoScriptComponent* Contender, float MouseDistanceToBillboard);

	void SwitchFocus(const UBangoScriptComponent* NewFocus);
};

UCLASS(Abstract, NotBlueprintable)
class UBangoDebugDraw_ScriptComponent : public UObject
{
	GENERATED_BODY()
	
public:
	UBangoDebugDraw_ScriptComponent();

protected:
	static void DebugDrawEditor(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent);
	
	static void DebugDrawEditorImpl(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent, float Alpha, const FVector& BillboardScreenPos);
	
	static void DebugDrawPIE(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent);
	
	static void DebugDrawPIEImpl(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos);
	
	static void DrawRunScriptInPIEWidget(FBangoDebugDrawCanvas& Canvas, UBangoScriptComponent* ScriptComponent, float Alpha, float MouseDistSqrd, const FVector& BillboardScreenPos);
	
private:
	static FBangoDebugDraw_ScriptComponentHover HoverInfo;
};
