#pragma once

#include "BangoScriptHandle.generated.h"

USTRUCT(BlueprintType)
struct BANGOSCRIPTS_API FBangoScriptHandle
{
	GENERATED_BODY()

	FBangoScriptHandle();

	FBangoScriptHandle(FGuid InGuid);
	
public:
	static FBangoScriptHandle GetNullHandle()
	{
		static FBangoScriptHandle NullHandle = FBangoScriptHandle(FGuid(0, 0, 0, 0));
		return NullHandle;
	}
	
	static FBangoScriptHandle NewHandle()
	{
		FGuid NewGuid = FGuid::NewGuid();
		
		// For that one in a trillion chance...
		while (NewGuid == ExpiredGuid)
		{
			NewGuid = FGuid::NewGuid();
		}
		
		return FBangoScriptHandle(NewGuid);
	}
	
private:
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
	FGuid Guid;

	static FGuid ExpiredGuid; 
	
	friend uint32 GetTypeHash(const FBangoScriptHandle& ScriptHandle)
	{
		return GetTypeHash(ScriptHandle.Guid);
	}

public:
	bool operator==(const FBangoScriptHandle& Other) const
	{
		return Guid == Other.Guid;
	}

	bool IsNull() const;
	
	bool IsRunning() const;
	
	bool IsExpired() const;
	
	void Expire();
	
	void Invalidate();
};