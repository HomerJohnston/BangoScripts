// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "BangoBlueprintFunctionLibrary.generated.h"

class UBangoScript;
struct FBangoScriptHandle;

UCLASS()
class UBangoBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    UFUNCTION(BlueprintCallable, DisplayName = "Abort Script", meta = (WorldContext = "WorldContext"))
    static void AbortScript(UObject* WorldContext, UPARAM(Ref) FBangoScriptHandle& Handle);
};
