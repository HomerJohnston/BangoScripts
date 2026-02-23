#include "BangoPIEActorLocationLookupSubsystem.h"

#include "Editor.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "WorldPartition/WorldPartition.h"
#include "GameFramework/Actor.h"
#include "Math/Vector.h"

void UBangoPIEActorLocationLookupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// TODO: ACTOR STREAMABLE REFS
	// FBangoEditorDelegates::RequestActorLocation.BindUObject(this, &ThisClass::OnRequestActorLocation);
}

void UBangoPIEActorLocationLookupSubsystem::OnRequestActorLocation(TSoftObjectPtr<AActor> Target, bool& bSuccess, FVector& OutLocation)
{
	if (UWorldPartition* WorldPartition = GEditor->EditorWorld->GetWorldPartition())
	{
		const FWorldPartitionActorDescInstance* ActorDesc = WorldPartition->GetActorDescInstanceByPath(Target.ToSoftObjectPath());
		
		if (ActorDesc)
		{
			OutLocation = ActorDesc->GetActorTransform().GetLocation();
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutLocation.ToString());
			bSuccess = true;
		}
	}
	
	bSuccess = false;
}
