#include "BangoScripts/Core/BangoScriptHandle.h"

FGuid FBangoScriptHandle::ExpiredGuid = FGuid(0, 0, 0, 1); 

FBangoScriptHandle::FBangoScriptHandle()
{
}

FBangoScriptHandle::FBangoScriptHandle(FGuid InGuid) : Guid(InGuid)
{
}

bool FBangoScriptHandle::IsNull() const
{
	return !Guid.IsValid();
}

bool FBangoScriptHandle::IsRunning() const
{
	return Guid.IsValid() && Guid != ExpiredGuid;
}

bool FBangoScriptHandle::IsExpired() const
{
	return Guid == ExpiredGuid;
}

void FBangoScriptHandle::Expire()
{
	Guid = ExpiredGuid;
}

void FBangoScriptHandle::Invalidate()
{
	Guid.Invalidate();
}
