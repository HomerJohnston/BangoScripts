#include "BangoScripts/EditorTooling/BangoHelpers.h"

#include "Editor.h"
#include "Components/ActorComponent.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"

#if WITH_EDITOR
bool Bango::Editor::IsComponentInEditedLevel(UObject* Object, EBangoAllowInvalid AllowInvalid)
{
	if (!GEditor)
		return false;
		
	if (GIsPlayInEditorWorld)
		return false;
	
	UWorld* World = Object->GetWorld();
	
	if (!World)
		return false;

	if (World->IsGameWorld())
		return false;

	if (World->bIsTearingDown)
		return false;

	if (Object->HasAnyFlags(RF_Transient | RF_ClassDefaultObject | RF_DefaultSubObject))
		return false;
	
	if (AllowInvalid == RequireValid)
	{
		if (Object->HasAnyFlags(RF_MirroredGarbage | RF_BeginDestroyed | RF_FinishDestroyed))
			return false;
	}
	
	if (Object->GetPackage() == GetTransientPackage())
		return false;
	
	// Additional checks if the thing is an actor component
	if (UActorComponent* Component = Cast<UActorComponent>(Object))
	{
		AActor* Actor = Component->GetOwner();
	
		if (!Actor || Actor->IsTemplate())
			return false;
		
		if (Actor->HasAnyFlags(RF_Transient))
			return false;
	
		if (Component->IsDefaultSubobject())
		{
			if (World->IsPlayInEditor())
				return false;
		
			return true;
		}
		else
		{
			ULevel* Level = Component->GetComponentLevel();
	
			if (!Level)
				return false;
	
			if (Level->GetPackage() == GetTransientPackage())
				return false;
	
			if (Component->GetComponentLevel() == nullptr)
				return false;
	
			return true;
		}
	}
	
	return true;
}

/*
bool Bango::Editor::IsComponentBeingDeleted(UActorComponent* Component)
{
	// 1. Must be in editor world
	if (GIsPlayInEditorWorld || Component->GetWorld() == nullptr || Component->GetWorld()->IsGameWorld())
		return false;

	// 2. Component must be explicitely removed from its owner
	if (Component->GetOwner() && !Component->GetOwner()->GetComponents().Contains(Component))
		return true;

	// 3. NOT deleted because PIE ended
	if (GEditor && GEditor->PlayWorld != nullptr)
		return false;

	// 4. NOT deleted because a Blueprint reinstance/recompile replaced it
	if (Component->HasAnyFlags(RF_Transient) && Component->HasAnyFlags(RF_ClassDefaultObject) == false)
		return false;

	return false;
}
*/

/*
FName Bango::Editor::GetBangoName(AActor* Actor)
{
	UBangoActorIDComponent* IDComponent = GetActorIDComponent(Actor);
	
	return IDComponent ? IDComponent->GetBangoName() : NAME_None;
}
*/
#endif
