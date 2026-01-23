#include "BangoScripts/Core/BangoScriptBlueprint.h"

#if WITH_EDITOR
#include "Editor.h"
#endif
#include "BangoScripts/Components/BangoScriptComponent.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BangoScriptBlueprint)

#define LOCTEXT_NAMESPACE "BangoScripts"

// ----------------------------------------------

UBangoScriptBlueprint::UBangoScriptBlueprint()
{
#if WITH_EDITOR
	bForceFullEditor = true;
#endif
}

// ----------------------------------------------

#if WITH_EDITOR
bool UBangoScriptBlueprint::ShouldBeMarkedDirtyUponTransaction() const
{
	// return Super::ShouldBeMarkedDirtyUponTransaction();

	// TODO spend some time investigating if this might break anything. If I leave this on, then "undoing" duplication or creation of a script component can throw an odd ensure message about blueprint compilation.
	return false;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
const TSoftObjectPtr<AActor> UBangoScriptBlueprint::GetOwnerActor() const
{
	return TSoftObjectPtr<AActor>(FSoftObjectPath(OwnerActorPath));
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
const IBangoScriptHolderInterface* UBangoScriptBlueprint::GetScriptHolder() const
{
	FSoftObjectPath ObjectPath(ScriptHolderObjectPath);
	TSoftObjectPtr<UObject> ObjectSoft = TSoftObjectPtr<UObject>(ObjectPath);
	
	UObject* Object = ObjectSoft.Get();
	
	if (Object)
	{
		return Cast<IBangoScriptHolderInterface>(Object);
	}
	
	return nullptr;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptBlueprint::SetOwnerActor(AActor* Actor)
{
	// This should only ever be called once in the asset's lifespan
	check(OwnerActorPath.IsEmpty());
	check(Actor);
	
	OwnerActorPath = Actor->GetPathName();
}

#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptBlueprint::SetScriptHolder(IBangoScriptHolderInterface& ScriptHolder)
{
	ScriptHolderObjectPath = ScriptHolder._getUObject()->GetPathName();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
const FGuid& UBangoScriptBlueprint::GetScriptGuid()
{
	check(ScriptGuid.IsValid()); 
	return ScriptGuid;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptBlueprint::SetScriptGuid(FGuid InGuid)
{
	// This should only ever be called once in the asset's lifespan
	check(!ScriptGuid.IsValid()); 
	
	Modify();
	ScriptGuid = InGuid;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void UBangoScriptBlueprint::Reset()
{
	//check(!ActorReference.IsEmpty());
	//check(ScriptGuid.IsValid());
	
	OwnerActorPath.Empty();
	ScriptGuid.Invalidate();
}

TWeakObjectPtr<UObject> UBangoScriptBlueprint::GetCurrentObjectBeingDebugged() const
{
	return CurrentObjectBeingDebugged;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
UBangoScriptBlueprint* UBangoScriptBlueprint::GetBangoScriptBlueprintFromClass(const TSoftClassPtr<UBangoScript> ScriptClass)
{
	const FSoftObjectPath& ScriptClassPath = ScriptClass.ToSoftObjectPath();
	
	if (ScriptClassPath.IsNull())
	{
		return nullptr;
	}
	
	if (!FPackageName::DoesPackageExist(ScriptClassPath.GetLongPackageName()))
	{
		//return nullptr;	
	}
	
	UClass* Class = ScriptClass.LoadSynchronous();
	
	if (!Class)
	{
		return nullptr;
	}
	
	if (!Class->ClassGeneratedBy)
	{
		return nullptr;
	}
	
	return Cast<UBangoScriptBlueprint>(Class->ClassGeneratedBy);
}
#endif

#undef LOCTEXT_NAMESPACE