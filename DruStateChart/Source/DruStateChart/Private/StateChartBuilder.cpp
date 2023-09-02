// Copyright Andrei Sudarikov. All Rights Reserved.

#include "StateChartBuilder.h"
#include "StateChartAsset.h"
#include "Impl/StateChartElements.h"
#include "Algo/AnyOf.h"

TObjectPtr<UStateChartAsset> FStateChartBuilder::Build(UObject* InParent, UClass* InClass, FName InName, EObjectFlags Flags)
{
    using namespace DruStateChart_Impl;

    if (StateChart != nullptr)
    {
        // return existing StateChart if already built
        return StateChart;
    }

    // we need StateBuilders to be sorted, because we use this order to autodetect initial transitions
    StateBuilders.Sort([&](auto& A, auto& B)
    {
        return A->SortOrder < B->SortOrder;
    });

    // build Lookups
    auto BuildLookups = [&](auto& Array)
    {
        for (auto& State : Array)
        {
            NameBuilder.Reset();
            AppendName(*State);
            FString Path = NameBuilder.ToString();

            PathLookup.Emplace(Path, &*State);
            IDLookup.Emplace(&*State, GuidFromString(Path));
        }
    };

    IDLookup.Emplace(&RootStateBuilder, GuidFromString(RootStateBuilder.Name));

    BuildLookups(StateBuilders);
    BuildLookups(ParallelBuilders);
    BuildLookups(HistoryBuilders);

    // fixup arguments, in case they were left with default values
    if (InParent == nullptr)
    {
        InParent = GetTransientPackage();
    }

    if (InClass == nullptr)
    {
        InClass = UStateChartAsset::StaticClass();
    }

    // create resulting state chart
    StateChart = NewObject<UStateChartAsset>(InParent, InClass, InName, Flags);

    auto CreateStateDefinitions = [&](auto& Array)
    {
        for (auto& State : Array)
        {
            StateChart->AllStates.Add(CreateStateDefinition(*State));
        }
    };

    // create definition for root state
    StateChart->AllStates.Add(CreateStateDefinition(RootStateBuilder));

    // create defintions for all other states
    CreateStateDefinitions(StateBuilders);
    CreateStateDefinitions(ParallelBuilders);
    CreateStateDefinitions(HistoryBuilders);

    // create definitions for all transitions
    for (auto& Transition : TransitionBuilders)
    {
        StateChart->AllTransitions.Add(CreateTransitionDefinition(*Transition));
    }

    // create initial transitions for "regular" states
    CreateInitialTransition(RootStateBuilder, RootStateBuilder.InitialStatePath);
    for (auto StateBuilder : StateBuilders)
    {
        CreateInitialTransition(*StateBuilder, StateBuilder->InitialStatePath);
    }

    // create initial transitions for History states
    for (auto HistoryBuilder : HistoryBuilders)
    {
        CreateHistoryTransition(*HistoryBuilder);
    }

    // make sure node tree is already assembled
    // we do it here, because we may be called from async loading thread. this way we are not wasting time of game thread
    StateChart->AssembleNodeTree();

    return StateChart;
}

void FStateChartBuilder::AppendName(const DruStateChart_Impl::FBuilderBase& Item, const TCHAR* RelativeName)
{
    auto AppendHelper = [&](auto Str)
    {
        if (NameBuilder.Len() > 0)
        {
            NameBuilder.AppendChar(TEXT('.'));
        }

        NameBuilder.Append(Str);
    };

    if (Item.Parent)
    {
        AppendName(*Item.Parent);
    }

    if (&Item != &RootStateBuilder)
    {
        AppendHelper(Item.Name);
    }

    if (RelativeName != nullptr)
    {
        AppendHelper(RelativeName);
    }
}

FGuid FStateChartBuilder::GuidFromString(const FString& Str)
{
    // create GUID from MD5 hash of a string
    union
    {
        uint32 Parts[4];
        uint8 Digest[16];
    } Bytes;

    FMD5 Md5Gen;

    Md5Gen.Update((uint8*)TCHAR_TO_ANSI(*Str), FCString::Strlen(*Str));
    Md5Gen.Final(Bytes.Digest);

    return FGuid(Bytes.Parts[0], Bytes.Parts[1], Bytes.Parts[2], Bytes.Parts[3]);
}

TObjectPtr<UCompoundStateDefinition> FStateChartBuilder::CreateStateDefinition(DruStateChart_Impl::FRootBuilder& Builder)
{
    TObjectPtr<UCompoundStateDefinition> Result = NewObject<UCompoundStateDefinition>(StateChart);

    Result->ID = IDLookup[&Builder];
    Result->FriendlyName = Builder.Name;
    Result->ParentID = IDLookup.FindRef(Builder.Parent);

    return Result;
}

TObjectPtr<UCompoundStateDefinition> FStateChartBuilder::CreateStateDefinition(DruStateChart_Impl::FStateBuilder& Builder)
{
    TObjectPtr<UCompoundStateDefinition> Result = NewObject<UCompoundStateDefinition>(StateChart);

    Result->ID = IDLookup[&Builder];
    Result->FriendlyName = Builder.Name;
    Result->ParentID = IDLookup.FindRef(Builder.Parent);
    Result->EnterActions = MoveTemp(Builder.EnterActions);
    Result->ExitActions = MoveTemp(Builder.ExitActions);
    Result->Handlers = MoveTemp(Builder.Handlers);
    Result->SortOrder = Builder.SortOrder;

    return Result;
}

TObjectPtr<UParallelStateDefinition> FStateChartBuilder::CreateStateDefinition(DruStateChart_Impl::FParallelBuilder& Builder)
{
    TObjectPtr<UParallelStateDefinition> Result = NewObject<UParallelStateDefinition>(StateChart);

    Result->ID = IDLookup[&Builder];
    Result->FriendlyName = Builder.Name;
    Result->ParentID = IDLookup.FindRef(Builder.Parent);
    Result->EnterActions = MoveTemp(Builder.EnterActions);
    Result->ExitActions = MoveTemp(Builder.ExitActions);
    Result->Handlers = MoveTemp(Builder.Handlers);
    Result->SortOrder = Builder.SortOrder;

    return Result;
}

TObjectPtr<UHistoryStateDefinition> FStateChartBuilder::CreateStateDefinition(DruStateChart_Impl::FHistoryBuilder& Builder)
{
    TObjectPtr<UHistoryStateDefinition> Result = NewObject<UHistoryStateDefinition>(StateChart);

    Result->ID = IDLookup[&Builder];
    Result->FriendlyName = Builder.Name;
    Result->ParentID = IDLookup.FindRef(Builder.Parent);
    Result->HistoryType = Builder.Type;

    return Result;
}

TObjectPtr<UTransitionDefinition> FStateChartBuilder::CreateTransitionDefinition(DruStateChart_Impl::FTransitionBuilder& Builder)
{
    TObjectPtr<UTransitionDefinition> Result = NewObject<UTransitionDefinition>(StateChart);

    Result->SourceState = IDLookup[Builder.Parent];
    Result->SortOrder = Builder.SortOrder;
    Result->bInitial = Builder.bInitial;
    Result->EventID = Builder.EventStruct;
    Algo::Transform(Builder.TargetPaths, Result->TargetStates, [&](const FString& Path) { return IDLookup[PathLookup[Path]]; });
    Result->Conditions = MoveTemp(Builder.Conditions);
    Result->Actions = MoveTemp(Builder.Actions);

    return Result;
}

void FStateChartBuilder::CreateInitialTransition(const DruStateChart_Impl::FBuilderBase& StateBuilder, const FString& InitialStatePath)
{
    TObjectPtr<UBaseStateDefinition>* Definition = StateChart->AllStates.FindByPredicate([&](auto S) { return S->ID == IDLookup[&StateBuilder]; });

    UCompoundStateDefinition* CompoundState = Cast<UCompoundStateDefinition>(*Definition);
    auto* FirstChild = StateChart->AllStates.FindByPredicate([&](TObjectPtr<UBaseStateDefinition> S)
    {
        return S->IsA<UActivatableStateDefinition>() && S->ParentID == CompoundState->ID;
    });

    // check that we have at least one child
    if (FirstChild)
    {
        // check if we already have initial transition
        bool bInitialFound = Algo::AnyOf(StateChart->AllTransitions, [&](auto Transition) { return Transition->SourceState == CompoundState->ID && Transition->bInitial; });
        if (!bInitialFound)
        {
            TObjectPtr<UTransitionDefinition> NewInitialTransition = NewObject<UTransitionDefinition>(StateChart);
            NewInitialTransition->SourceState = CompoundState->ID;
            NewInitialTransition->bInitial = true;

            if (!InitialStatePath.IsEmpty())
            {
                // set transition target to specified state
                NameBuilder.Reset();
                AppendName(StateBuilder, *InitialStatePath);

                NewInitialTransition->TargetStates.Add(IDLookup[PathLookup[NameBuilder.ToString()]]);
            }
            else
            {
                // set transition target tofirst child state
                NewInitialTransition->TargetStates.Add((*FirstChild)->ID);
            }

            StateChart->AllTransitions.Emplace(NewInitialTransition);
        }
    }
}

void FStateChartBuilder::CreateHistoryTransition(const DruStateChart_Impl::FHistoryBuilder& HistoryBuilder)
{
    TObjectPtr<UBaseStateDefinition>* Definition = StateChart->AllStates.FindByPredicate([&](auto S) { return S->ID == IDLookup[&HistoryBuilder]; });

    UHistoryStateDefinition* HistoryState = Cast<UHistoryStateDefinition>(*Definition);

    if (!HistoryBuilder.InitialStatePath.IsEmpty())
    {
        TObjectPtr<UTransitionDefinition> NewInitialTransition = NewObject<UTransitionDefinition>(StateChart);
        NewInitialTransition->SourceState = HistoryState->ID;

        NameBuilder.Reset();
        AppendName(*HistoryBuilder.Parent, *HistoryBuilder.InitialStatePath);

        NewInitialTransition->TargetStates.Add(IDLookup[PathLookup[NameBuilder.ToString()]]);
        StateChart->AllTransitions.Emplace(NewInitialTransition);
    }
    else
    {
        // pick up initial transition of parent state
        TObjectPtr<UTransitionDefinition>* ParentInitialTransition = StateChart->AllTransitions.FindByPredicate(
            [&, ParentID = IDLookup[HistoryBuilder.Parent]](TObjectPtr<UTransitionDefinition> T)
        {
            return T->SourceState == ParentID && T->bInitial;
        });

        TObjectPtr<UTransitionDefinition> NewInitialTransition = DuplicateObject(*ParentInitialTransition, StateChart);
        NewInitialTransition->SourceState = HistoryState->ID;
        NewInitialTransition->bInitial = false;
        StateChart->AllTransitions.Emplace(NewInitialTransition);
    }
}