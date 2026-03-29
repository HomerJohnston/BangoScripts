#pragma once

#include "BangoScriptObjectPath.generated.h"

/*
 * Simple wrapper for a string which represents an FSoftObjectPath. We use a string because
 * 
 * Includes a property type customization to have an actor picker like a normal path field.
 */
USTRUCT()
struct BANGOSCRIPTS_API FBangoScriptObjectPath
{
    GENERATED_BODY()

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere)
    FString Path;
#endif

#if WITH_EDITOR
    bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar);

    void operator=(const FString& OtherPath);
    
    bool IsEmpty();
    
    void Empty();
    
    operator FString() const;
#endif
};

#if WITH_EDITOR
template<>
struct TStructOpsTypeTraits<FBangoScriptObjectPath> : public TStructOpsTypeTraitsBase2<FBangoScriptObjectPath>
{
    enum
    {
        WithSerializeFromMismatchedTag = true,
    };
};
#endif