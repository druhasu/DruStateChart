// Copyright Andrei Sudarikov. All Rights Reserved.

#include "StateChartAssetActions.h"
#include "StateChartAsset.h"
#include "StateChartAssetEditor.h"

FText FStateChartAssetActions::GetName() const
{
    return INVTEXT("State Chart");
}

FColor FStateChartAssetActions::GetTypeColor() const
{
    return FColor::Magenta;
}

UClass* FStateChartAssetActions::GetSupportedClass() const
{
    return UStateChartAsset::StaticClass();
}

void FStateChartAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
    FStateChartAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

uint32 FStateChartAssetActions::GetCategories()
{
    return EAssetTypeCategories::Misc;
}