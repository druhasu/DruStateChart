// Copyright Andrei Sudarikov. All Rights Reserved.

#include "StateChartAssetEditor.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "StateChartAsset.h"
#include "Slate/SStatesView.h"

const FName FStateChartAssetEditor::AppIdentifier(TEXT("StateChartAssetEditorApp"));
const FName FStateChartAssetEditor::ToolkitName(TEXT("StateChartAssetEditorToolkit"));
const FName FStateChartAssetEditor::StatesTabId(TEXT("StateChartAssetEditor_StatesTab"));
const FName FStateChartAssetEditor::GraphTabId(TEXT("StateChartAssetEditor_GraphTab"));
const FName FStateChartAssetEditor::PropertiesTabId(TEXT("StateChartAssetEditor_PropertiesTab"));

TSharedRef<FStateChartAssetEditor> FStateChartAssetEditor::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit)
{
    TSharedRef<FStateChartAssetEditor> NewEditor = MakeShared<FStateChartAssetEditor>();
    NewEditor->InitEditor(Mode, InitToolkitHost, ObjectsToEdit);

    return NewEditor;
}

void FStateChartAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(NSLOCTEXT("DruStateChartEditor", "WorkspaceMenu_GenericAssetEditor", "Asset Editor"));

    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(StatesTabId, FOnSpawnTab::CreateSP(this, &FStateChartAssetEditor::SpawnStatesTab))
        .SetDisplayName(NSLOCTEXT("DruStateChartEditor", "StatesTab", "States"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

    InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FStateChartAssetEditor::SpawnGraphTab))
        .SetDisplayName(NSLOCTEXT("DruStateChartEditor", "GraphTab", "Graph"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

    InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FStateChartAssetEditor::SpawnPropertiesTab))
        .SetDisplayName(NSLOCTEXT("DruStateChartEditor", "PropertiesTab", "Details"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FStateChartAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

    InTabManager->UnregisterTabSpawner(PropertiesTabId);
}

FName FStateChartAssetEditor::GetToolkitFName() const
{
    return ToolkitName;
}

FText FStateChartAssetEditor::GetBaseToolkitName() const
{
    return NSLOCTEXT("DruStateChartEditor", "AppLabel", "StateChart Asset Editor");
}

FString FStateChartAssetEditor::GetWorldCentricTabPrefix() const
{
    return TEXT("StateChart Editor ");
}

FLinearColor FStateChartAssetEditor::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.5f, 0.0f, 0.0f, 0.5f);
}

void FStateChartAssetEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit)
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    StatesView = SNew(SStatesView, Cast<UStateChartAsset>(ObjectsToEdit[0]))
        .SelectionChanged_Lambda([&](UBaseStateDefinition* State, auto) { DetailsView->SetObject(State); });

    const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_StateChartAssetEditor_Layout_v1_Test1")
    ->AddArea
    (
        FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
        ->Split
        (
            FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.2f)
                ->AddTab(StatesTabId, ETabState::OpenedTab)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->AddTab(GraphTabId, ETabState::OpenedTab)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.2f)
                ->AddTab(PropertiesTabId, ETabState::OpenedTab)
            )
        )
    );

    const bool bCreateDefaultStandaloneMenu = true;
    const bool bCreateDefaultToolbar = true;
    FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, AppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectsToEdit);

    RegenerateMenusAndToolbars();
}

TSharedRef<SDockTab> FStateChartAssetEditor::SpawnStatesTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == StatesTabId);

    return
        SNew(SDockTab)
        .Label(NSLOCTEXT("DruStateChartEditor", "StatesTitle", "States"))
        .TabColorScale(GetTabColorScale())
        .OnCanCloseTab_Lambda([]() { return false; })
        .CanEverClose(false)
        [
            StatesView.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FStateChartAssetEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == GraphTabId);

    return
        SNew(SDockTab)
        .Label(NSLOCTEXT("DruStateChartEditor", "GraphTitle", "Graph"))
        .TabColorScale(GetTabColorScale())
        .OnCanCloseTab_Lambda([]() { return false; })
        .CanEverClose(false)
        [
            SNew(STextBlock).Text(INVTEXT("Graph WIP"))
        ];
}

TSharedRef<SDockTab> FStateChartAssetEditor::SpawnPropertiesTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == PropertiesTabId);

    return
        SNew(SDockTab)
        .Label(NSLOCTEXT("DruStateChartEditor", "DetailsTitle", "Details"))
        .TabColorScale(GetTabColorScale())
        .OnCanCloseTab_Lambda([]() { return false; })
        .CanEverClose(false)
        [
            DetailsView.ToSharedRef()
        ];
}
