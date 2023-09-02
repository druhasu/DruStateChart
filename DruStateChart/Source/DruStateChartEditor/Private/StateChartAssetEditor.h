// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"

class FStateChartAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	static TSharedRef<FStateChartAssetEditor> CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit);

	void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	FName GetToolkitFName() const override;
	FText GetBaseToolkitName() const override;
	FString GetWorldCentricTabPrefix() const override;
	FLinearColor GetWorldCentricTabColorScale() const override;

private:
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit);

	/* Create the States tab and its content */
	TSharedRef<SDockTab> SpawnStatesTab(const FSpawnTabArgs& Args);

	/* Create the Graph tab and its content */
	TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);

	/* Create the Properties tab and its content */
	TSharedRef<SDockTab> SpawnPropertiesTab(const FSpawnTabArgs& Args);

	/* App Identifier */
	static const FName AppIdentifier;
	static const FName ToolkitName;

	/* The tab ids for all the tabs used */
	static const FName StatesTabId;
	static const FName GraphTabId;
	static const FName PropertiesTabId;

	/* States View*/
	TSharedPtr<class SStatesView> StatesView;

	/* Details view */
	TSharedPtr<class IDetailsView> DetailsView;
};