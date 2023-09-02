// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Slate/SStatesView.h"
#include "Slate/SStateTreeItem.h"
#include "StateChartAsset.h"

void SStatesView::Construct(const FArguments& Args, TObjectPtr<UStateChartAsset> InStateChart)
{
    StateChart = InStateChart;

    BuildTreeData();

    ChildSlot
    [
        SAssignNew(TreeView, SStatesTreeView)
        .ItemHeight(22)
        .TreeItemsSource(&RootStates)
        .SelectionMode(ESelectionMode::Single)
        .OnGenerateRow(this, &ThisClass::MakeRow)
        .OnGetChildren(this, &ThisClass::GetStateChildren)
        .OnSelectionChanged(Args._SelectionChanged)
    ];
}

void SStatesView::BuildTreeData()
{
    RootStates.Reset();
    TreeData.Reset();

    auto& Nodes = StateChart->GetAssembledNodes();

    for (auto State : StateChart->AllStates)
    {
        if (!State->ParentID.IsValid())
        {
            RootStates.Add(State);
        }
        else
        {
            TArray<UBaseStateDefinition*>& Siblings = TreeData.FindOrAdd(Nodes.StateIDToDefinition[State->ParentID]);
            Siblings.Add(State);
        }
    }
}

TSharedRef<ITableRow> SStatesView::MakeRow(UBaseStateDefinition* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SStateTreeItem, Item, OwnerTable);
}

void SStatesView::GetStateChildren(UBaseStateDefinition* Item, TArray<UBaseStateDefinition*>& OutChildren)
{
    OutChildren = TreeData.FindRef(Item);
}