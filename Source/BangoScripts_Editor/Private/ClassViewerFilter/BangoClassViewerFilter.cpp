#include "BangoClassViewerFilter.h"

#include "ClassViewerModule.h"
#include "Editor.h"
#include "PropertyHandle.h"
#include "BangoScripts/Core/BangoScript.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "Private/Utilities/BangoEditorUtility.h"
#include "Misc/PackageName.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "UObject/Package.h"
#include "UObject/Class.h"
#include "Widgets/BangoScriptClassViewerNode.h"

// ----------------------------------------------

FBangoScriptsGlobalClassViewerFilter::FBangoScriptsGlobalClassViewerFilter(TSharedPtr<IClassViewerFilter> InChildFilter)
{
	ChildFilter = InChildFilter;
}
// ----------------------------------------------

bool FBangoScriptsGlobalClassViewerFilter::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (ChildFilter && !ChildFilter->IsClassAllowed(InInitOptions, InClass, InFilterFuncs))
	{
		return false;
	}
	
	if (InClass->IsChildOf(UBangoScript::StaticClass()))
	{
		return false;
	}
	
	return true;
}

// ----------------------------------------------

bool FBangoScriptsGlobalClassViewerFilter::IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (ChildFilter && !ChildFilter->IsUnloadedClassAllowed(InInitOptions, InUnloadedClassData, InFilterFuncs))
	{
		return false;
	}
		
	if (InUnloadedClassData->IsChildOf(UBangoScript::StaticClass()))
	{
		return false;
	}
	
	return true;
}

FBangoScriptsScriptContainerClassViewerFilter::FBangoScriptsScriptContainerClassViewerFilter(const FClassViewerInitializationOptions& InInitOptions) :
	TextFilter(MakeShared<FTextFilterExpressionEvaluator>(ETextFilterExpressionEvaluatorMode::BasicString)),
	FilterFunctions(MakeShared<FClassViewerFilterFuncs>()),
	AssetRegistry(FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get())
{
	// Create a game-specific filter, if the referencing property/assets were supplied
	if (GEditor)
	{
		FAssetReferenceFilterContext AssetReferenceFilterContext;
		AssetReferenceFilterContext.AddReferencingAssets(InInitOptions.AdditionalReferencingAssets);
		AssetReferenceFilterContext.AddReferencingAssetsFromPropertyHandle(InInitOptions.PropertyHandle);
		AssetReferenceFilter = GEditor->MakeAssetReferenceFilter(AssetReferenceFilterContext);
	}
	
	// MetaData class filter
	FProperty* Property = InInitOptions.PropertyHandle.IsValid() ? InInitOptions.PropertyHandle->GetProperty() : nullptr;
	if (Property && Property->GetOwnerProperty()->HasMetaData("GetClassFilter"))
	{
		const FString& GetClassFilterFunctionName = Property->GetOwnerProperty()->GetMetaData("GetClassFilter");
		if (!GetClassFilterFunctionName.IsEmpty())
		{
			if (const UClass* OuterBaseClass = InInitOptions.PropertyHandle->GetOuterBaseClass())
			{
				UObject* OuterBaseCDO = OuterBaseClass->GetDefaultObject<UObject>();
				const UFunction* GetClassFilterFunction = OuterBaseCDO ? OuterBaseCDO->FindFunction(*GetClassFilterFunctionName) : nullptr;
				if (ensure(GetClassFilterFunction && GetClassFilterFunction->HasAllFunctionFlags(FUNC_Static)))
				{
					FilterDelegate = FOnShouldFilterClass::CreateUFunction(OuterBaseCDO, GetClassFilterFunction->GetFName());
				}
			}
		}
	}
}

// ================================================================================================

bool FBangoScriptsScriptContainerClassViewerFilter::IsNodeAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<FBangoScriptClassViewerNode>& Node)
{
	if (Node->Class.IsValid())
	{
		return IsClassAllowed(InInitOptions, Node->Class.Get(), FilterFunctions);
	}
	else if (InInitOptions.bShowUnloadedBlueprints && Node->UnloadedBlueprintData.IsValid())
	{
		return IsUnloadedClassAllowed(InInitOptions, Node->UnloadedBlueprintData.ToSharedRef(), FilterFunctions);
	}

	return false;
}

// ----------------------------------------------

bool FBangoScriptsScriptContainerClassViewerFilter::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (InClass->IsChildOf(UBangoScript::StaticClass()))
	{
		if (UBangoScriptBlueprint* Blueprint = UBangoScriptBlueprint::GetBangoScriptBlueprintFromClass(InClass))
		{
			if (Blueprint->GetName().StartsWith(Bango::Editor::GetLevelScriptNamePrefix()))
			{
				return false;
			}
			
			UPackage* BlueprintPackage = Blueprint->GetPackage();
			
			if (BlueprintPackage == GetTransientPackage())
			{
				return false;
			}
			
			FString PackagePath = FPackageName::GetLongPackagePath(BlueprintPackage->GetName());
			
			return !PackagePath.StartsWith(Bango::Editor::GetGameScriptRootFolder());
		}
	}
	
	return false;
}

// ----------------------------------------------

bool FBangoScriptsScriptContainerClassViewerFilter::IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (!InUnloadedClassData->IsChildOf(UBangoScript::StaticClass()))
	{
		return false;
	}

	const TSharedPtr<const FString> ClassName = InUnloadedClassData->GetClassName();
	
	if (ClassName->StartsWith(Bango::Editor::GetLevelScriptNamePrefix()))
	{
		// Do not allow dropdown selection of level scripts
		return false;
	}
	
	FTopLevelAssetPath AssetPath = InUnloadedClassData->GetClassPathName();
	
	if (AssetPath.ToString().StartsWith(Bango::Editor::GetGameScriptRootFolder()))
	{
		// Do not allow dropdown selection of level scripts
		return false;
	}
	
	return true;
}

// ----------------------------------------------
