#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"

#include "BangoScripts/Core/BangoScriptContainer.h"
#include "Templates/SubclassOf.h"

// ----------------------------------------------

#if WITH_EDITOR
FBangoScriptContainer& IBangoScriptHolderInterface::GetScriptContainer()
{
	static FBangoScriptContainer NullScriptContainer;
	checkNoEntry();
	
	return NullScriptContainer;
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
const FBangoScriptContainer& IBangoScriptHolderInterface::GetScriptContainer() const
{
	IBangoScriptHolderInterface* MutableThis = const_cast<IBangoScriptHolderInterface*>(this);
	return MutableThis->GetScriptContainer();
}
#endif

// ----------------------------------------------

#if WITH_EDITOR
FVector IBangoScriptHolderInterface::GetDebugDrawOrigin() const
{
	return FVector::ZeroVector;
}

const FString& IBangoScriptHolderInterface::GetStartEventComment() const
{
	static const FString DefaultComment = "";
	return DefaultComment;
}
#endif

// ----------------------------------------------
