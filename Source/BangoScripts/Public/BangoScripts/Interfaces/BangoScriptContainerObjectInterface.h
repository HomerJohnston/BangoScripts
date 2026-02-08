#pragma once
#include "UObject/Interface.h"

#include "BangoScriptContainerObjectInterface.generated.h"

class UBangoScript;
struct FBangoScriptContainer;

UINTERFACE(MinimalAPI, NotBlueprintable)
class UBangoScriptHolderInterface : public UInterface
{
	GENERATED_BODY()
};

/*
 * I don't know if it will ever be possible to implement generic UObjects as script containers; it's incredibly difficult to handle duplication/undo/redo correctly.
 * This interface represents the first very basic step towards it, but it is not expected that anyone would want to try using this publicly.
 */
class BANGOSCRIPTS_API IBangoScriptHolderInterface
{
	GENERATED_BODY()

public:
	
#if WITH_EDITOR
	virtual FBangoScriptContainer& GetScriptContainer();
	
	const FBangoScriptContainer& GetScriptContainer() const;

	// Should be in relative local space to an owning actor
	virtual FVector GetDebugDrawOrigin() const;
	
	virtual const FString& GetStartEventComment() const;
	
	virtual void UpdateBillboard() {};
	
	virtual void LogStatus(FString* OutString = nullptr) const {};
#endif
};
