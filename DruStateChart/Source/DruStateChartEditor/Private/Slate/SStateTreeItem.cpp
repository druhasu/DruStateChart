// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Slate/SStateTreeItem.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

void SStateTreeItem::Construct(const FArguments& InArgs, UBaseStateDefinition* InDefinition, const TSharedRef<STableViewBase>& InOwnerTableView)
{
    Definition = InDefinition;

    STableRow<UBaseStateDefinition*>::FArguments Args = STableRow<UBaseStateDefinition*>::FArguments()
    [
        SNew(SBox)
        .HeightOverride(22)
        .VAlign(VAlign_Center)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(16)
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(IPropertyTypeCustomizationUtils::GetBoldFont())
                    .Text(GetStateType())
                ]
            ]

            + SHorizontalBox::Slot()
            [
                SNew(SInlineEditableTextBlock)
                .IsSelected(this, &ThisClass::IsSelectedExclusively)
                .Text(this, &ThisClass::GetStateName)
                .OnTextCommitted(this, &ThisClass::SetStateName)
            ]
        ]
    ];

    STableRow<UBaseStateDefinition*>::Construct(Args, InOwnerTableView);
}

FText SStateTreeItem::GetStateType() const
{
    static TMap<UClass*, FText, TFixedSetAllocator<4>> Map
    {
        { UHistoryStateDefinition::StaticClass(),  INVTEXT("H") },
        { UFinalStateDefinition::StaticClass(),    INVTEXT("F") },
        { UCompoundStateDefinition::StaticClass(), INVTEXT("S") },
        { UParallelStateDefinition::StaticClass(), INVTEXT("P") },
    };

    return Map.FindRef(Definition->GetClass());
}

FText SStateTreeItem::GetStateName() const
{
    return FText::FromString(Definition->FriendlyName);
}

void SStateTreeItem::SetStateName(const FText& NewName, ETextCommit::Type CommitType)
{
    if (CommitType != ETextCommit::OnCleared)
    {
        Definition->FriendlyName = NewName.ToString();
    }
}