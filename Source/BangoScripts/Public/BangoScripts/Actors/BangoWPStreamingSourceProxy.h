#pragma once

#include "GameFramework/Actor.h"
#include "WorldPartition/WorldPartitionStreamingSource.h"

#include "BangoWPStreamingSourceProxy.generated.h"

// TODO: ACTOR STREAMABLE REFS
/**
 * Not currently in use.
 */
UCLASS()
class ABangoWPStreamingSourceProxy : public AActor, public IWorldPartitionStreamingSourceProvider
{
	GENERATED_BODY()
	
public:
	ABangoWPStreamingSourceProxy();
	
protected:
	bool bEnabled;
	
	/** Color used for debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldPartition, meta = (EditCondition = "bEnabled"))
	FColor StreamingSourceDebugColor;

	/** PlayerController streaming source priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldPartition, meta = (EditCondition = "bEnabled"))
	EStreamingSourcePriority StreamingSourcePriority;

	/** Optional aggregated shape list used to build a custom shape for the streaming source. When empty, fallbacks sphere shape with a radius equal to grid's loading range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldPartition, meta = (EditCondition = "bEnabled"))
	TArray<FStreamingSourceShape> StreamingSourceShapes;

	/** Whether the PlayerController streaming source should block on slow streaming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldPartition, meta = (EditCondition = "bEnabled"))
	bool bStreamingSourceShouldBlockOnSlowStreaming;

	/** Whether the PlayerController streaming source should activate cells after loading. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldPartition, meta=(EditCondition="bEnabled"))
	bool bStreamingSourceShouldActivate;

public:
	void SetEnabled(bool bInEnabled)
	{
		bEnabled = bInEnabled;
	}
	
	bool IsStreamingSourceEnabled() const
	{
		return bEnabled;
	};
	
	/**
	* Whether the PlayerController streaming source should block on slow streaming.
	* Default implementation returns bStreamingSourceShouldBlockOnSlowStreaming but can be overriden in child classes.
	* @return true if it should.
	*/
	UFUNCTION(BlueprintCallable, Category = WorldPartition)
	virtual bool StreamingSourceShouldBlockOnSlowStreaming() const { return bEnabled && bStreamingSourceShouldBlockOnSlowStreaming; }

protected:
	
	void BeginPlay() override;
	
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual bool GetStreamingSourcesInternal(TArray<FWorldPartitionStreamingSource>& OutStreamingSources) const;
	
	bool GetStreamingSources(TArray<FWorldPartitionStreamingSource>& OutStreamingSources) const override;
	
	bool GetStreamingSource(FWorldPartitionStreamingSource& OutStreamingSource) const override;
	
	const UObject* GetStreamingSourceOwner() const override
	{
		return this;
	}
	
	/**
	* Gets the streaming source priority.
	* Default implementation returns StreamingSourceShapes but can be overriden in child classes.
	* @return the streaming source priority.
	*/
	UFUNCTION(BlueprintCallable, Category = WorldPartition)
	virtual void GetStreamingSourceShapes(TArray<FStreamingSourceShape>& OutShapes) const;
	
	/**
	* Whether the PlayerController streaming source should activate cells after loading.
	* Default implementation returns bStreamingSourceShouldActivate but can be overriden in child classes.
	* @return true if it should.
	*/
	UFUNCTION(BlueprintCallable, Category = WorldPartition)
	virtual bool StreamingSourceShouldActivate() const { return bEnabled && bStreamingSourceShouldActivate; }

	

}; 