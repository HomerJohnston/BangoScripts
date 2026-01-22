#include "BangoScripts/Core/BangoScriptContainer.h"

#include "BangoScripts/Core/BangoScript.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

// ----------------------------------------------

FBangoScriptContainer::FBangoScriptContainer()
{
}

#if WITH_EDITOR
void FBangoScriptContainer::SetGuid(const FGuid& InGuid)
{
	check(!Guid.IsValid());
	check(InGuid.IsValid());
		
	Guid = InGuid;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::Unset()
{
	Guid.Invalidate();
	ScriptClass = nullptr;
	ScriptClass.Reset();
	ScriptInputs.Reset();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::SetScriptClass(TSubclassOf<UObject> NewScriptClass)
{
	ScriptClass = NewScriptClass;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
bool FBangoScriptContainer::AreScriptInputsOutDated() const
{
	// Limit checking to once per tick
	if (GFrameCounter == CachedFrameCheck)
	{
		return CachedResult;
	}
	
	TArray<FProperty*> MissingProperties;
	TArray<FName> DeadProperties;
	
	GetPropertiesForRefresh(MissingProperties, DeadProperties);

	FBangoScriptContainer& MutableThis = *const_cast<FBangoScriptContainer*>(this);
	MutableThis.CachedFrameCheck = GFrameCounter;
	MutableThis.CachedResult = MissingProperties.Num() > 0 || DeadProperties.Num() > 0;
	
	return CachedResult;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::UpdateScriptInputs()
{
	TArray<FProperty*> MissingProperties;
	TArray<FName> DeadProperties;
	
	GetPropertiesForRefresh(MissingProperties, DeadProperties);

	// Modify(); // TODO, IMPORTANT 
	ScriptInputs.RemovePropertiesByName(DeadProperties);
	
	for (FProperty* Property : MissingProperties)
	{
		//Modify(); // TODO, IMPORTANT
		ScriptInputs.AddProperty(Property->GetFName(), Property);
	}
}

// ----------------------------------------------

void FBangoScriptContainer::GetPropertiesForRefresh(TArray<FProperty*>& MissingProperties, TArray<FName>& DeadProperties) const
{
	if (GetScriptClass().IsValid())
	{
		TSubclassOf<UBangoScript> Script = GetScriptClass().Get();
		
		for (TFieldIterator<FProperty> PropertyIterator(Script); PropertyIterator; ++PropertyIterator)
		{
			FProperty* Property = *PropertyIterator;
			
			if (Property->HasAnyPropertyFlags(CPF_Edit | CPF_ExposeOnSpawn) && !Property->IsNative())
			{
				MissingProperties.Add(Property);
			}
		}
	}
	
	int32 NumBagProperties = ScriptInputs.GetNumPropertiesInBag();
	
	const UPropertyBag* BagStruct = ScriptInputs.GetPropertyBagStruct();

	if (BagStruct)
	{
		for (int32 i = 0; i < NumBagProperties; ++i)
		{
			const FPropertyBagPropertyDesc* Desc = BagStruct->FindPropertyDescByIndex(i);
		
			if (MissingProperties.ContainsByPredicate( [Desc] (const FProperty* Property)
			{
				if (Desc->Name != Property->GetFName())
					return false;
				
				if (!Property->SameType(Desc->CachedProperty))
					return false;
				
				return true;
			} ))
			{
				// We already have this property. We won't need to add it.
				MissingProperties.RemoveAll([Desc] (const FProperty* Property) { return Desc->Name == Property->GetFName(); } );
			}
			else
			{
				// This property exists, but it's not supposed to. We need to remove it.
				DeadProperties.Add(Desc->Name);
			}
		}
	}
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::SetRequestedName(const FString& InName)
{
	RequestedName = InName;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
const FString& FBangoScriptContainer::GetRequestedName() const
{
	return RequestedName;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::SetNewLevelScriptRequested()
{
	bNewLeveScriptRequested = true;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::SetIsCreated()
{
	bCreated = true;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
void FBangoScriptContainer::SetIsDuplicate()
{
	bIsDuplicate = true;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
bool FBangoScriptContainer::ConsumeNewLevelScriptRequest()
{
	bool bRequest = bNewLeveScriptRequested;
	bNewLeveScriptRequested = false;
	return bRequest;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
bool FBangoScriptContainer::ConsumeCreated()
{
	bool bRequest = bCreated;
	bCreated = false;
	return bRequest;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
bool FBangoScriptContainer::ConsumeDuplicate()
{
	bool bWasDuplicate = bIsDuplicate;
	bIsDuplicate = false;
	return bWasDuplicate;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
const FString& FBangoScriptContainer::GetDescription() const
{
	return Description;
}
#endif

// ----------------------------------------------
