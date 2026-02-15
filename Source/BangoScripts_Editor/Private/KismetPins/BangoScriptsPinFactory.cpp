#include "BangoScriptsPinFactory.h"

#include "SGraphPinClass_BangoScript.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoRunScript.h"

TSharedPtr<class SGraphPin> FBangoScriptsPinFactory::CreatePin(class UEdGraphPin* Pin) const
{
	static FName YapPinMetaKey("YapPin");
	static FName RunScriptPinName("Script");
	
	if (Pin->GetFName() == RunScriptPinName)
	{
		if (UK2Node_BangoRunScript* RunScriptNode = Cast<UK2Node_BangoRunScript>(Pin->GetOuter()))
		{
			return SNew(SGraphPinClass_BangoScript, Pin);
		}
	}
	
	return nullptr;
}
