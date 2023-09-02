// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "StateChartAssetActions.h"

class FDruStateChartEditorModule : public IModuleInterface
{
public:
    using ThisClass = FDruStateChartEditorModule;

    void StartupModule() override
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

        AssetTools.RegisterAssetTypeActions(MakeShared<FStateChartAssetActions>());
    }
};

IMPLEMENT_MODULE(FDruStateChartEditorModule, DruStateChartEditor)