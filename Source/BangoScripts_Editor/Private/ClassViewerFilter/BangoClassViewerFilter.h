#pragma once

#include "ClassViewerFilter.h"

class FTextFilterExpressionEvaluator;
class FBangoScriptClassViewerNode;

/**
 * This class filters out BangoScripts from the new blueprints dialog entirely. New scripts should be created usin gthe Content new-asset context menu.
 */
class FBangoScriptsGlobalClassViewerFilter : public IClassViewerFilter
{
public:
	FBangoScriptsGlobalClassViewerFilter(TSharedPtr<IClassViewerFilter> InChildFilter);
	
protected:
	TSharedPtr<IClassViewerFilter> ChildFilter;

public:
	bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override;
	bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const class IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override;
};

/**
 * This class filters the script selection list inside of FBangoScriptContainer. It is used by the property type customization.
 */
class FBangoScriptsScriptContainerClassViewerFilter : public IClassViewerFilter
{
public:
	FBangoScriptsScriptContainerClassViewerFilter(const FClassViewerInitializationOptions& InInitOptions);

	/**
	 * This function checks whether a node passes the filter defined by IsClassAllowed/IsUnloadedClassAllowed.
	 *
	 * @param InInitOptions				The Class Viewer/Picker options.
	 * @param Node						The Node to check.
	 * @param bCheckTextFilter			Whether to check the TextFilter. Disabling it could be useful e.g., to verify that the parent class of a IsNodeAllowed() object is also valid (regardless of the TextFilter, which will likely fail to pass).
	 */
	bool IsNodeAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<FBangoScriptClassViewerNode>& Node, const bool bCheckTextFilter);

	bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override;
	
	//bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs, const bool bCheckTextFilter);
	bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const class IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override;
	//bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const class IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs, const bool bCheckTextFilter);

	TArray<UClass*> InternalClasses;
	TArray<FDirectoryPath> InternalPaths;
	FOnShouldFilterClass FilterDelegate;
	TSharedRef<FTextFilterExpressionEvaluator> TextFilter;
	TSharedRef<FClassViewerFilterFuncs> FilterFunctions;
	TSharedPtr<IAssetReferenceFilter> AssetReferenceFilter;
	const IAssetRegistry& AssetRegistry;
};
