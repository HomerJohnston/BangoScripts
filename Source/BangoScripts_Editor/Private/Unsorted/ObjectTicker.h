// 
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"

class UObject;

template <typename T>
struct TObjectTickFunction : public FTickFunction
{

	/*
	Called when tick is executed.

	@param float - Delta Time.
	@param ELevelTick - Tick Type.
	@param ENamedThreads::Type - Thread.
	@param const FGraphEventRef& - Executing graph.
	@return void
	*/
	virtual void ExecuteTick(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent) override;


	void operator=(const TObjectTickFunction<T>& OtherTickFunction)
	{
		Owner = OtherTickFunction.Owner;
	}

	/* Owner of this tick function. */
	T* Owner = nullptr;

};

/**
* TObjectTicker is a CRTP class to enable ticks for arbitrary objects.
* You must register the tick function with a valid world.
*/
template <typename T>
struct TObjectTicker
{

	/**
	* Constructor.
	*/
	TObjectTicker()
	{
		TickFunction.bAllowTickOnDedicatedServer = false;
		TickFunction.bCanEverTick = true;
		TickFunction.TickGroup = TG_PostPhysics;
		TickFunction.Owner = (T*)(this);
	}

	/**
	* Virtual destructor.
	*/
	virtual ~TObjectTicker() = default;

	/**
	* Tick function.
	* 
	* @param float - Delta time.
	* @param fELevelTick
	* @param fENamedThreads::Type
	* @param fconst FGraphEventRef&
	* @return void
	*/
	virtual void Tick(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent) = 0;

private:

	/**
	* Internal tick execution function. Do not call!
	* 
	* @param float - Delta time.
	* @param fELevelTick
	* @param fENamedThreads::Type
	* @param fconst FGraphEventRef&
	* @return void
	*/
	virtual void ExecuteTickInternal(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent) final;

public:

	/* Tick function owned by this ticker */
	TObjectTickFunction<T> TickFunction;

	friend struct TObjectTickFunction<T>;

};

template<typename T>
inline void TObjectTickFunction<T>::ExecuteTick(
	float DeltaTime,
	ELevelTick TickType,
	ENamedThreads::Type CurrentThread,
	const FGraphEventRef& MyCompletionGraphEvent)
{
	if (IsValid(Owner))
	{
		Owner->ExecuteTickInternal(DeltaTime, TickType, CurrentThread, MyCompletionGraphEvent);
	}
}

template<typename T>
inline void TObjectTicker<T>::ExecuteTickInternal(
	float DeltaTime, 
	ELevelTick TickType, 
	ENamedThreads::Type CurrentThread, 
	const FGraphEventRef& MyCompletionGraphEvent)
{
	Tick(DeltaTime, TickType, CurrentThread, MyCompletionGraphEvent);
}