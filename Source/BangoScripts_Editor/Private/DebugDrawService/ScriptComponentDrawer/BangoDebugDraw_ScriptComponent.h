#pragma once

#include "BangoDebugDraw_ScriptComponent.generated.h"

class AActor;
class APlayerController;
class UBangoScriptComponent;
class UCanvas;
class USceneComponent;
class FBangoScriptBlueprintEditor;
struct FBangoDebugDrawCanvas;

UCLASS(Abstract, NotBlueprintable)
class UBangoDebugDraw_ScriptComponent : public UObject
{
	GENERATED_BODY()
	
public:
	UBangoDebugDraw_ScriptComponent();

protected:
	static void DebugDrawEditor(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent);
	
	static void DebugDrawEditorImpl(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent, float Alpha, const FVector& BillboardScreenPos);
	
	static void DebugDrawPIE(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent);
	
	static void DebugDrawPIEImpl(FBangoDebugDrawCanvas& Canvas, const UBangoScriptComponent* ScriptComponent, float Alpha, const FVector& BillboardScreenPos);
};
