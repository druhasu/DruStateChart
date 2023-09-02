// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Impl/StateChartBuilderOps.h"
#include "UObject/ObjectPtr.h"

class UStateChartAsset;
class UCompoundStateDefinition;
class UParallelStateDefinition;
class UHistoryStateDefinition;
class UTransitionDefinition;

class DRUSTATECHART_API FStateChartBuilder
{
public:
    DruStateChart_Impl::FRootBuilder& Root()
    {
        return RootStateBuilder;
    }

    TObjectPtr<UStateChartAsset> Build(UObject* InParent = nullptr, UClass* InClass = nullptr, FName InName = FName(), EObjectFlags Flags = EObjectFlags::RF_NoFlags);

    DruStateChart_Impl::FStateBuilder& State(FString Name)
    {
        return *StateBuilders.Emplace_GetRef(MakeShared<DruStateChart_Impl::FStateBuilder>(MoveTemp(Name)));
    }

    DruStateChart_Impl::FParallelBuilder& Parallel(FString Name)
    {
        return *ParallelBuilders.Emplace_GetRef(MakeShared<DruStateChart_Impl::FParallelBuilder>(MoveTemp(Name)));
    }

    DruStateChart_Impl::FHistoryBuilder& History(FString Name)
    {
        return *HistoryBuilders.Emplace_GetRef(MakeShared<DruStateChart_Impl::FHistoryBuilder>(MoveTemp(Name)));
    }

    DruStateChart_Impl::FTransitionBuilder& Transition()
    {
        return *TransitionBuilders.Emplace_GetRef(MakeShared<DruStateChart_Impl::FTransitionBuilder>());
    }

private:
    using FPathLookup = TMap<FString, DruStateChart_Impl::FBuilderBase*>;
    using FIDLookup = TMap<DruStateChart_Impl::FBuilderBase*, FGuid>;

    void AppendName(const DruStateChart_Impl::FBuilderBase& Item, const TCHAR* RelativeName = nullptr);
    FGuid GuidFromString(const FString& Str);

    TObjectPtr<UCompoundStateDefinition> CreateStateDefinition(DruStateChart_Impl::FRootBuilder& Builder);
    TObjectPtr<UCompoundStateDefinition> CreateStateDefinition(DruStateChart_Impl::FStateBuilder& Builder);
    TObjectPtr<UParallelStateDefinition> CreateStateDefinition(DruStateChart_Impl::FParallelBuilder& Builder);
    TObjectPtr<UHistoryStateDefinition> CreateStateDefinition(DruStateChart_Impl::FHistoryBuilder& Builder);
    TObjectPtr<UTransitionDefinition> CreateTransitionDefinition(DruStateChart_Impl::FTransitionBuilder& Builder);

    void CreateInitialTransition(const DruStateChart_Impl::FBuilderBase& StateBuilder, const FString& InitialStatePath);
    void CreateHistoryTransition(const DruStateChart_Impl::FHistoryBuilder& HistoryBuilder);

    TArray<TSharedRef<DruStateChart_Impl::FStateBuilder>> StateBuilders;
    TArray<TSharedRef<DruStateChart_Impl::FParallelBuilder>> ParallelBuilders;
    TArray<TSharedRef<DruStateChart_Impl::FHistoryBuilder>> HistoryBuilders;
    TArray<TSharedRef<DruStateChart_Impl::FTransitionBuilder>> TransitionBuilders;

    DruStateChart_Impl::FRootBuilder RootStateBuilder;

    TStringBuilder<128> NameBuilder;

    FIDLookup IDLookup;
    FPathLookup PathLookup;

    TObjectPtr<UStateChartAsset> StateChart = nullptr;
};
