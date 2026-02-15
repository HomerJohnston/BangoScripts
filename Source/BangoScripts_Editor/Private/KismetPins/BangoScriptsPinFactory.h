#pragma once

#include "EdGraphUtilities.h"

class FBangoScriptsPinFactory : public FGraphPanelPinFactory
{
	TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};