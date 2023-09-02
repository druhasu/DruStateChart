// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Impl/StateChartElements.h"

class UStateChartAsset;

class SStatesView : public SCompoundWidget
{
public:

    using SStatesTreeView = STreeView<UBaseStateDefinition*>;
    using ThisClass = SStatesView;

    SLATE_BEGIN_ARGS(SStatesView) {}

        SLATE_EVENT(SStatesTreeView::FOnSelectionChanged, SelectionChanged)

    SLATE_END_ARGS()

    void Construct(const FArguments& Args, TObjectPtr<UStateChartAsset> InStateChart);

private:
    void BuildTreeData();

    TSharedRef<ITableRow> MakeRow(UBaseStateDefinition* Item, const TSharedRef<STableViewBase>& OwnerTable);
    void GetStateChildren(UBaseStateDefinition* Item, TArray<UBaseStateDefinition*>& OutChildren);

    TWeakObjectPtr<UStateChartAsset> StateChart;

    TSharedPtr<SStatesTreeView> TreeView;
    TArray<UBaseStateDefinition*> RootStates;
    TMap<UBaseStateDefinition*, TArray<UBaseStateDefinition*>> TreeData;
};