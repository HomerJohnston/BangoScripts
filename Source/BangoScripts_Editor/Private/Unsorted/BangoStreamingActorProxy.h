#pragma once

#include "GameFramework/Actor.h"

#include "BangoStreamingActorProxy.generated.h"


/**
 * TODO: ACTOR STREAMABLE REFS
 * 
 * This is not currently in use.
 * 
 * This is part of an experiment to apply streaming sources to actor reference nodes
 */
UCLASS()
class ABangoStreamingProxyActor : public AActor
{
	GENERATED_BODY()
	
public:
	ABangoStreamingProxyActor();
	
#if WITH_EDITOR
	void OnConstruction(const FTransform& Transform) override;
#endif
};
