// Copyright Andrei Sudarikov. All Rights Reserved.

#include "StateChartAsset.h"
#include "Impl/StateChartElements.h"
#include "ExternalPackageHelper.h"

TObjectPtr<UStateChartAsset> UStateChartAsset::Create(const TArray<TObjectPtr<UBaseStateDefinition>>& States, const TArray<TObjectPtr<UTransitionDefinition>>& Transitions)
{
    TObjectPtr<UStateChartAsset> Result = NewObject<UStateChartAsset>();
    Result->AllStates = States;
    Result->AllTransitions = Transitions;
    Result->AssembleNodeTree();

    return Result;
}

void UStateChartAsset::Serialize(FStructuredArchive::FRecord Record)
{
    Super::Serialize(Record);

    FArchive& Ar = Record.GetUnderlyingArchive();
    if (Ar.IsSaving())
    {
        if (Ar.IsCooking())
        {
            // embed all states and transitions inside this asset when cooking
            Record << SA_VALUE(TEXT("AllStates"), AllStates);
            Record << SA_VALUE(TEXT("AllTransitions"), AllTransitions);
        }
        else
        {
            // otherwise save empty array
            TArray<TObjectPtr<UBaseStateDefinition>> EmptyStates;
            TArray<TObjectPtr<UTransitionDefinition>> EmptyTransitions;

            Record << SA_VALUE(TEXT("AllStates"), EmptyStates);
            Record << SA_VALUE(TEXT("AllTransitions"), EmptyTransitions);
        }
    }
    else
    {
        Record << SA_VALUE(TEXT("AllStates"), AllStates);
        Record << SA_VALUE(TEXT("AllTransitions"), AllTransitions);
    }
}

void UStateChartAsset::PostLoad()
{
    Super::PostLoad();

#if WITH_EDITOR
    FExternalPackageHelper::LoadObjectsFromExternalPackages<UStateChartElementAsset>(this, [&](UStateChartElementAsset* Object)
    {
        if (UBaseStateDefinition* State = Cast<UBaseStateDefinition>(Object))
        {
            AllStates.Add(State);
        }
        else if (UTransitionDefinition* Transition = Cast<UTransitionDefinition>(Object))
        {
            AllTransitions.Add(Transition);
        }
    });
#endif
}

const StateChart_Impl::FStateChartNodes& UStateChartAsset::GetAssembledNodes() const
{
    if (bNodesDirty)
    {
        const_cast<UStateChartAsset*>(this)->AssembleNodeTree();
    }

    return Nodes;
}

void UStateChartAsset::AssembleNodeTree()
{
    // remove null entries
    AllStates.Remove(nullptr);
    AllTransitions.Remove(nullptr);

    Nodes.CreateNodes(AllStates, AllTransitions);

    bNodesDirty = false;
}

#if WITH_EDITOR
void UStateChartAsset::MoveSubObjectsToExternalPackage()
{
    for (TObjectPtr<UBaseStateDefinition> Definition : AllStates)
    {
        if (Definition && !Definition->IsPackageExternal())
        {
            FExternalPackageHelper::SetPackagingMode(Definition, this, true, false, EPackageFlags::PKG_None);
        }
    }

    for (TObjectPtr<UTransitionDefinition> Definition : AllTransitions)
    {
        if (Definition && !Definition->IsPackageExternal())
        {
            FExternalPackageHelper::SetPackagingMode(Definition, this, true, false, EPackageFlags::PKG_None);
        }
    }

    bNodesDirty = true;
}
#endif
