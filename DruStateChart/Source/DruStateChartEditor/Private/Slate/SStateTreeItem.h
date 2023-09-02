// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Impl/StateChartElements.h"

class SStateTreeItem : public STableRow<UBaseStateDefinition*>
{
public:
    using ThisClass = SStateTreeItem;

    SLATE_BEGIN_ARGS(SStateTreeItem) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UBaseStateDefinition* InDefinition, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
    FText GetStateType() const;

    FText GetStateName() const;
    void SetStateName(const FText& NewName, ETextCommit::Type CommitType);


    TWeakObjectPtr<UBaseStateDefinition> Definition;
};