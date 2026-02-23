#include "BangoWPStreamingSourceProxy.h"

#include "Engine/World.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

// ----------------------------------------------

ABangoWPStreamingSourceProxy::ABangoWPStreamingSourceProxy()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
	bEnabled = false;
	bStreamingSourceShouldActivate = true;
}

// ----------------------------------------------

void ABangoWPStreamingSourceProxy::BeginPlay()
{
	Super::BeginPlay();
	
	UWorld* World = GetWorld();
	
	if (UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>())
	{
		WorldPartitionSubsystem->RegisterStreamingSourceProvider(this);
	}
}

// ----------------------------------------------

void ABangoWPStreamingSourceProxy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();
	
	if (UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>())
	{
		WorldPartitionSubsystem->UnregisterStreamingSourceProvider(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

// ----------------------------------------------

bool ABangoWPStreamingSourceProxy::GetStreamingSourcesInternal(TArray<FWorldPartitionStreamingSource>& OutStreamingSources) const
{
	FWorldPartitionStreamingSource& StreamingSource = OutStreamingSources.AddDefaulted_GetRef();
	
	StreamingSource.Location = GetActorLocation();
	StreamingSource.Rotation = GetActorRotation();
	
	const ENetMode NetMode = GetNetMode();
	const bool bIsServer = (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer);
	
	StreamingSource.Name = GetFName();
	StreamingSource.TargetState = StreamingSourceShouldActivate() ? EStreamingSourceTargetState::Activated : EStreamingSourceTargetState::Loaded;
	StreamingSource.bBlockOnSlowLoading = StreamingSourceShouldBlockOnSlowStreaming();
	StreamingSource.DebugColor = StreamingSourceDebugColor;
	StreamingSource.Priority = StreamingSourcePriority;
	StreamingSource.bRemote = !bIsServer;
	
	// GetStreamingSourceShapes(StreamingSource.Shapes);
	
	return true;
}

// ----------------------------------------------

bool ABangoWPStreamingSourceProxy::GetStreamingSource(FWorldPartitionStreamingSource& OutStreamingSource) const
{
	checkNoEntry();
	return false;
}

// ----------------------------------------------

void ABangoWPStreamingSourceProxy::GetStreamingSourceShapes(TArray<FStreamingSourceShape>& OutShapes) const
{
	if (StreamingSourceShapes.Num())
	{
		OutShapes.Append(StreamingSourceShapes);
	}
	else
	{
		// TODO make these a project setting
		FStreamingSourceShape DefaultShape;
		DefaultShape.Location = GetActorLocation();
		DefaultShape.Rotation = GetActorRotation();
		DefaultShape.bUseGridLoadingRange = false;
		DefaultShape.Radius = 0.0f;
		
		OutShapes.Emplace(DefaultShape);
	}
}

// ----------------------------------------------

bool ABangoWPStreamingSourceProxy::GetStreamingSources(TArray<FWorldPartitionStreamingSource>& OutStreamingSources) const
{
	const ENetMode NetMode = GetNetMode();
	
	const bool bIsServer = (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer || NetMode == NM_Standalone);
	
	if (IsStreamingSourceEnabled() && bIsServer)
	{
		return GetStreamingSourcesInternal(OutStreamingSources);
	}
	
	return false;
}

// ----------------------------------------------
