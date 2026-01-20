#pragma once

#if WITH_EDITOR
#include "Editor.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "BangoScripts/EditorTooling/BangoHelpers.h"
#endif

#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"

#if 0

/**  WIP -- works for editor viewport non-PIE debug drawing // TODO implement PIE debug drawing and update this
 * 
 * Usage: 
 * 1) Inherit this onto your AActorComponent class
 * 2) Override OnRegister
 *   - After Super::OnRegister, call BangoDebugDraw_Register<ThisClass>(this);
 * 3) Override OnUnregister
 *   - Before Super::OnUnregister, call BangoDebugDraw_Unregister(this);
 * 4) Override DebugDrawEditor and implement
 * 5) Optionally override GetLabelHeight to return a UPROPERTY or other logic
 */
class FBangoDebugDrawServiceBase
{
#if WITH_EDITOR
public:
	virtual ~FBangoDebugDrawServiceBase() = default;
	
protected:
	template <typename T>
	void BangoDebugDraw_Register(T* Component);
	
	void BangoDebugDraw_Unregister(UActorComponent* Component);

	
private:
	void __DebugDrawEditor(UCanvas* Canvas, APlayerController* PlayerController) const;
	
	void __DebugDrawGame(UCanvas* Canvas, APlayerController* PlayerController) const;
	
	void __DebugDrawDispatch(UCanvas* Canvas, TDelegate<void(UCanvas*, FVector, float)> Delegate) const;
	
	virtual void DebugDrawEditor(UCanvas* Canvas, FVector ScreenLocation, float Alpha) const { };
	
	virtual void DebugDrawGame(UCanvas* Canvas, FVector ScreenLocation, float Alpha) const { };
	
protected:
	float MinDistance = 2000.0f;
	float MaxDistance = 2500.0f;
	float DefaultLabelHeight = 200.0f;
	
	// This should be overridden in all user classes, use this to avoid having different labels overlap each other. This is kind of ghetto but it's easy and it works.
	// float LabelOffset = 0.0f;
	
	// Override this to return a UPROPERTY value
	// virtual float GetLabelHeight() const { return DefaultLabelHeight + LabelOffset; }
#endif
};

// ==============================================

#if WITH_EDITOR
template <typename T>
void FBangoDebugDrawServiceBase::BangoDebugDraw_Register(T* Component)
{
	check(Component);
	
	if (Component->IsTemplate())
	{
		return;
	}

	if (Bango::Editor::IsComponentInEditedLevel(Component))
	{
		FBangoEditorDelegates::BangoDebugDrawEditor.AddUObject(Component, &T::__DebugDrawEditor);	
	
		FEditorDelegates::PreBeginPIE.AddWeakLambda(Component, [Component] (const bool PIE)
		{
			FBangoEditorDelegates::BangoDebugDrawEditor.RemoveAll(Component);
		});	
		
		FEditorDelegates::PrePIEEnded.AddWeakLambda(Component, [Component] (const bool PIE)
		{
			FBangoEditorDelegates::BangoDebugDrawEditor.AddUObject(Component, &T::__DebugDrawEditor);	
		});
	}
	else if (Component->GetWorld()->IsGameWorld())
	{
		FBangoEditorDelegates::DebugDrawRequest.AddUObject(Component, &T::__DebugDrawGame);
	}
}
#endif

// ----------------------------------------------

#endif
