#include "BangoScripts/Core/BangoScriptObjectPath.h"

#if WITH_EDITOR
bool FBangoScriptObjectPath::SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar)
{
    if (Tag.Type == NAME_StrProperty)
    {
        FString Value;
        Ar << Value;
        
        Path = Value;
        
        return true;
    }
    
    return false;
}
#endif

#if WITH_EDITOR
void FBangoScriptObjectPath::operator=(const FString& OtherPath)
{
    Path = OtherPath;
}
#endif

#if WITH_EDITOR
bool FBangoScriptObjectPath::IsEmpty()
{
    return Path.IsEmpty();
}
#endif

#if WITH_EDITOR
void FBangoScriptObjectPath::Empty()
{
    Path.Empty();
}
#endif

#if WITH_EDITOR
FBangoScriptObjectPath::operator FString() const
{
    return Path;
}
#endif