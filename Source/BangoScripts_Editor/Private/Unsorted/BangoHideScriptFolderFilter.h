#pragma once

#include "ContentBrowserDataFilter.h"

class FBangoHideScriptFolderFilter : public IContentBrowserHideFolderIfEmptyFilter
{
public:
	bool HideFolderIfEmpty(FName Path, FStringView PathString) const override
	{
		static const FString BangoScriptsPath("/Game" / Bango::Editor::GetGameScriptRootFolder());

		if (PathString.StartsWith(BangoScriptsPath))
		{
			return true;
		}

		return false;
	}
};
