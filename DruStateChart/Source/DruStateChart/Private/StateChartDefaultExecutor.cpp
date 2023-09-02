// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Impl/StateChartDefaultExecutor.h"
#include "StateChartAsset.h"
#include "StateChartAction.h"
#include "StateChartCondition.h"
#include "StateHandler.h"
#include "Impl/StateChartElements.h"
#include "Impl/StateChartNodes.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Algo/Copy.h"
#include "Algo/Transform.h"

namespace StateChart_Impl
{

template <typename T, typename TTargetAllocator, typename TSourceAllocator>
void AppendUnique(TArray<T, TTargetAllocator>& Target, const TArray<T, TSourceAllocator>& Source)
{
    for (auto& Item : Source)
    {
        Target.AddUnique(Item);
    }
}

FStateChartDefaultExecutor::FStateChartDefaultExecutor(UStateChartAsset& StateChartAsset, TObjectPtr<UObject> ContextObject)
    : Asset(&StateChartAsset)
    , Context(*this, ContextObject)
    , Nodes(&StateChartAsset.GetAssembledNodes())
{
}

void FStateChartDefaultExecutor::Execute()
{
    if (Nodes->StateNodes.Num() > 0)
    {
        // activate initial state
        StartNewPlan({ { Nodes->StateNodes[0].InitialTransitionIndex } }, {});

        // run all events
        ProcessEventsSynchronous();
    }
}

TArray<TObjectPtr<UBaseStateDefinition>> FStateChartDefaultExecutor::GetActiveStates() const
{
    TArray<TObjectPtr<UBaseStateDefinition>> Result;
    Algo::Transform(ActiveStates, Result, [this](FIndex Index) { return Nodes->StateNodes[Index].Definition; });
    return Result;
}

void FStateChartDefaultExecutor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(Asset);
    Collector.AddReferencedObject(Context.ContextObject);

    for (auto& EventStruct : ExternalEventQueue)
    {
        EventStruct.AddStructReferencedObjects(Collector);
    }
    
    for (auto& EventStruct : InternalEventQueue)
    {
        EventStruct.AddStructReferencedObjects(Collector);
    }

    if (bExecutingPlan)
    {
        CurrentPlan.Event.AddStructReferencedObjects(Collector);
    }
}

void FStateChartDefaultExecutor::ExecuteEventImpl(FConstStructView Event)
{
    if (bExecutingPlan)
    {
        // we'll process it later
        bInsideActionExecution ?
            InternalEventQueue.Emplace(Event) :
            ExternalEventQueue.Emplace(Event);

        return;
    }

    // no need to put event in queue, because CurrentPlan may not be null if there are any events in those queues
    // so we start it right away

    check(ExternalEventQueue.IsEmpty());
    check(InternalEventQueue.IsEmpty());

    StartNewPlan(CollectTransitions(Event.GetScriptStruct()), Event);
    ProcessEventsSynchronous();
}

void FStateChartDefaultExecutor::StartNewPlan(const FTransitionIndexArray& Transitions, FInstancedStruct Event)
{
    using namespace StateChart_Impl;

    check(!bExecutingPlan);

    if (Transitions.Num() != 0)
    {
        CurrentPlan = FExecutionPlan(CurrentPlan.PlanIndex + 1);
        CurrentPlan.Event = MoveTemp(Event);
        bExecutingPlan = true;

        FStateIndexArray Temp;

        CollectStatesToExit(Transitions, Temp);
        Temp.Sort([](auto A, auto B) { return B < A; });
        Algo::Transform(Temp, CurrentPlan.Steps, [](FIndex Index) { return FExecutionPlanStep{ EStepType::Exit, Index }; });

        RecordHistoryStates(Temp);
        Temp.Reset();

        Algo::Transform(Transitions, CurrentPlan.Steps, [](FIndex Index) { return FExecutionPlanStep{ EStepType::Transition, Index }; });

        CollectStatesToEnter(Transitions, Temp, CurrentPlan.StatesForDefaultEntry);
        Temp.Sort();
        Algo::Transform(Temp, CurrentPlan.Steps, [](FIndex Index) { return FExecutionPlanStep{ EStepType::Enter, Index }; });
    }
}

void FStateChartDefaultExecutor::ProcessEventsSynchronous()
{
    while (bExecutingPlan)
    {
        // process CurrentPlan, until completion or interruption by async action
        ProcessPlanSynchronous();

        if (bExecutingPlan)
        {
            // not finished synchronously, need to wait for continuation
            return;
        }

        FInstancedStruct Event;

        if (InternalEventQueue.Num() != 0)
        {
            Event = InternalEventQueue.PopFrontValue();
        }
        else if (ExternalEventQueue.Num() != 0)
        {
            Event = ExternalEventQueue.PopFrontValue();
        }

        if (Event.IsValid())
        {
            StartNewPlan(CollectTransitions(Event.GetScriptStruct()), Event);
        }
    }
}

void FStateChartDefaultExecutor::ProcessPlanSynchronous()
{
    check(bExecutingPlan);

    TGuardValue<bool> Guard(bInsideExecutionLoop, true);

    const int32 NumSteps = CurrentPlan.Steps.Num();

    uint16& StepIndex = CurrentPlan.StepIndex;
    EActionContinuationType& ContinuationType = CurrentPlan.ContinuationType;

    while (StepIndex < NumSteps)
    {
        // reset counter. it will be updated inside respective Exit/Enter functions
        CurrentPlan.NumActionsToComplete = 0;
        CurrentPlan.ContinuationDelegate = FSimpleDelegate::CreateSP(this, &FStateChartDefaultExecutor::OnActionCompleted, CurrentPlan.PlanIndex, StepIndex);

        auto& Step = CurrentPlan.Steps[StepIndex];

        switch (Step.Type)
        {
            case EStepType::Exit:
                ContinuationType = ExitStateAsync(Step.ObjectIndex);
                break;

            case EStepType::Transition:
                ContinuationType = ExecuteTransitionActionsAsync(Step.ObjectIndex);
                break;

            case EStepType::Enter:
                ContinuationType = EnterStateAsync(Step.ObjectIndex);
                break;
        }

        StepIndex++;

        if (ContinuationType != EActionContinuationType::Immediate)
        {
            break;
        }
    }

    if (StepIndex == NumSteps)
    {
        // all states processed
        bExecutingPlan = false;

        // empty arrays to free up memory, in case it was allocated in the heap
        CurrentPlan.Steps.Empty();
        CurrentPlan.StatesForDefaultEntry.Empty();
    }
}

void FStateChartDefaultExecutor::RecordHistoryStates(const FStateIndexArray& StatesToExit)
{
    for (FIndex StateIndex : StatesToExit)
    {
        ForEachChild(StateIndex, [&](FIndex ChildIndex, const FStateNode& ChildNode)
        {
            if (ChildNode.Type == EStateType::History)
            {
                FStateIndexArray NewHistory;

                if (ChildNode.GetDefinition<UHistoryStateDefinition>()->HistoryType == EHistoryType::Deep)
                {
                    Algo::CopyIf(ActiveStates, NewHistory, [&](FIndex Index)
                    {
                        const FStateNode& Node = Nodes->StateNodes[Index];
                        return Node.Type == EStateType::Atomic && IsDescendant(Index, StateIndex);
                    });
                }
                else
                {
                    Algo::CopyIf(ActiveStates, NewHistory, [&](FIndex Index)
                    {
                        const FStateNode& Node = Nodes->StateNodes[Index];
                        return Node.ParentIndex == StateIndex;
                    });
                }

                HistoryLookup.Emplace(ChildIndex, MoveTemp(NewHistory));
            }

            return true;
        });
    }
}

EActionContinuationType FStateChartDefaultExecutor::ExitStateAsync(FIndex StateIndex)
{
    const FStateNode& StateNode = Nodes->StateNodes[StateIndex];

    EActionContinuationType Result = EActionContinuationType::Immediate;

    // execute actions
    if (auto* CastedDefinition = StateNode.GetDefinition<UBaseStateWithActionsDefinition>())
    {
        Result = ExecuteAsyncActionList(CastedDefinition->ExitActions, Result);
    }

    // shutdown state handlers
    for (auto It = StateHandlers.CreateKeyIterator(StateIndex); It; ++It)
    {
        Result = ExecuteAsyncAction([&]() { return It.Value()->StateExitedAsync(Context, CurrentPlan.ContinuationDelegate); }, Result);
        It.Value()->MarkAsGarbage();
        It.RemoveCurrent();
    }

    ActiveStates.Remove(StateIndex);

    return Result;
}

EActionContinuationType FStateChartDefaultExecutor::ExecuteTransitionActionsAsync(FIndex TransitionIndex)
{
    EActionContinuationType Result = EActionContinuationType::Immediate;

    Result = ExecuteAsyncActionList(Nodes->TransitionNodes[TransitionIndex].Definition->Actions, Result);

    return Result;
}

EActionContinuationType FStateChartDefaultExecutor::EnterStateAsync(FIndex StateIndex)
{
    const FStateNode& StateNode = Nodes->StateNodes[StateIndex];
    ActiveStates.AddUnique(StateIndex);

    EActionContinuationType Result = EActionContinuationType::Immediate;

    // instantiate state handler
    if (auto* ActivatableState = StateNode.GetDefinition<UActivatableStateDefinition>())
    {
        for (TObjectPtr<UStateHandler> HandlerTemplate : ActivatableState->Handlers)
        {
            UStateHandler* InstancedHandler = DuplicateObject(HandlerTemplate, HandlerTemplate->GetOuter());
            StateHandlers.Add(StateIndex, InstancedHandler);

            StateHandlerCreatedDelegate.Broadcast(*InstancedHandler);

            Result = ExecuteAsyncAction([&]() { return InstancedHandler->StateEnteredAsync(CurrentPlan.Event, Context, CurrentPlan.ContinuationDelegate); }, Result);
        }
    }

    // execute enter actions
    if (auto* CastedDefinition = StateNode.GetDefinition<UBaseStateWithActionsDefinition>())
    {
        Result = ExecuteAsyncActionList(CastedDefinition->EnterActions, Result);
    }

    // execute initial transfition actions
    if (CurrentPlan.StatesForDefaultEntry.Contains(StateIndex))
    {
        Result = ExecuteAsyncActionList(Nodes->TransitionNodes[StateNode.InitialTransitionIndex].Definition->Actions, Result);
    }

    // unsupported yet
    if (StateNode.Type == EStateType::Final)
    {
        if (StateNode.ParentIndex.IsNone())
        {
            // running = false
        }
        else
        {

        }
    }

    return Result;
}

void FStateChartDefaultExecutor::OnActionCompleted(uint16 PlanIndex, uint16 StepIndex)
{
    if (bInsideExecutionLoop)
    {
        bLastActionExecutedSynchronously = true;

        // nothing more to do here.
        // rest will be handled by calling function
        return;
    }

    if (PlanIndex != CurrentPlan.PlanIndex || StepIndex < CurrentPlan.StepIndex - 1)
    {
        // this is an action from previous step or plan, we don't care about it anymore
        return;
    }

    check(StepIndex == CurrentPlan.StepIndex - 1);

    CurrentPlan.NumActionsToComplete -= 1;

    if (CurrentPlan.NumActionsToComplete != 0 && CurrentPlan.ContinuationType == EActionContinuationType::LastFinish)
    {
        // we still have some actions to complete before we can proceed to next step
        return;
    }

    // process next steps & events
    ProcessEventsSynchronous();
}

FStateChartDefaultExecutor::FTransitionIndexArray FStateChartDefaultExecutor::CollectTransitions(FConstStructView Event)
{
    FTransitionIndexArray Result;

    for (FIndex StateIndex : ActiveStates)
    {
        const FStateNode& StateNode = Nodes->StateNodes[StateIndex];
        if (StateNode.Type != EStateType::Atomic)
        {
            // iterate over Atomic nodes only
            continue;
        }

        auto CheckTransitions = [&](FIndex Index, const FStateNode& Node)
        {
            for (int32 TransitionIndex = 0; TransitionIndex < Node.NumTransitions; ++TransitionIndex)
            {
                const FTransitionNode& TransitionNode = Nodes->TransitionNodes[Node.TransitionIndex + TransitionIndex];
                if (TransitionNode.EventID == Event.GetScriptStruct())
                {
                    if (EvaluateConditions(TransitionNode, Event))
                    {
                        Result.Add(Node.TransitionIndex + TransitionIndex);
                        return false; // stop iteration, we found transition
                    }
                }
            }

            // continue searching
            return true;
        };

        if (CheckTransitions(StateIndex, StateNode))
        {
            ForEachParent(StateIndex, CheckTransitions);
        }
    }

    return RemoveConflictingTransitions(Result);
}

FStateChartDefaultExecutor::FTransitionIndexArray FStateChartDefaultExecutor::RemoveConflictingTransitions(const FTransitionIndexArray& Transitions)
{
    FTransitionIndexArray Result;

    for (FIndex TransitionIndex : Transitions)
    {
        bool bPreempted = false;
        FTransitionIndexArray TransitionsToRemove;

        for (FIndex SecondTransitionIndex : Result)
        {
            FStateIndexArray States1;
            FStateIndexArray States2;

            CollectStatesToExit({ { TransitionIndex } }, States1);
            CollectStatesToExit({ { SecondTransitionIndex } }, States2);

            if (Algo::AnyOf(States1, [&](FIndex Index) { return States2.Contains(Index); }))
            {
                if (IsDescendant(Nodes->TransitionNodes[TransitionIndex].SourceNodeIndex, Nodes->TransitionNodes[SecondTransitionIndex].SourceNodeIndex))
                {
                    TransitionsToRemove.AddUnique(SecondTransitionIndex);
                }
                else
                {
                    bPreempted = true;
                    break;
                }
            }
        }

        if (!bPreempted)
        {
            for (FIndex Index : TransitionsToRemove)
            {
                Result.Remove(Index);
            }

            Result.Add(TransitionIndex);
        }
    }

    return Result;
}

void FStateChartDefaultExecutor::CollectStatesToExit(const FTransitionIndexArray& Transitions, FStateIndexArray& OutStatesToExit) const
{
    for (FIndex TransitionIndex : Transitions)
    {
        const FTransitionNode& TransitionNode = Nodes->TransitionNodes[TransitionIndex];
        if (TransitionNode.Definition->TargetStates.Num() != 0)
        {
            FIndex DomainStateIndex = GetTransitionDomain(TransitionNode);

            for (FIndex ActiveStateIndex : ActiveStates)
            {
                if (IsDescendant(ActiveStateIndex, DomainStateIndex))
                {
                    OutStatesToExit.Add(ActiveStateIndex);
                }
            }
        }
    }
}

void FStateChartDefaultExecutor::CollectStatesToEnter(const FTransitionIndexArray& Transitions, FStateIndexArray& OutStatesToEnter, FStateIndexArray& OutStatesForDefaultEntry) const
{
    for (FIndex TransitionIndex : Transitions)
    {
        const FTransitionNode& TransitionNode = Nodes->TransitionNodes[TransitionIndex];
        for (const FGuid& StateID : TransitionNode.Definition->TargetStates)
        {
            AddDescendantStatesToEnter(Nodes->StateIDToNodeIndex[StateID], OutStatesToEnter, OutStatesForDefaultEntry);
        }

        FIndex Ancestor = GetTransitionDomain(TransitionNode);
        for (FIndex TargetStateIndex : GetTransitionEffectiveTargets(TransitionNode))
        {
            AddAncestorStatesToEnter(TargetStateIndex, Ancestor, OutStatesToEnter, OutStatesForDefaultEntry);
        }
    }
}

void FStateChartDefaultExecutor::AddDescendantStatesToEnter(FIndex StateIndex, FStateIndexArray& OutStatesToEnter, FStateIndexArray& OutStatesForDefaultEntry) const
{
    auto AddStatesToEnter = [&](const FStateIndexArray& StateIndexes, FIndex AncestorIndex)
    {
        for (FIndex Index : StateIndexes)
        {
            AddDescendantStatesToEnter(Index, OutStatesToEnter, OutStatesForDefaultEntry);
        }

        for (FIndex Index : StateIndexes)
        {
            AddAncestorStatesToEnter(Index, AncestorIndex, OutStatesToEnter, OutStatesForDefaultEntry);
        }
    };

    const FStateNode& StateNode = Nodes->StateNodes[StateIndex];

    if (StateNode.Type == EStateType::History)
    {
        const FStateIndexArray* HistoryTargetStates = HistoryLookup.Find(StateIndex);
        if (HistoryTargetStates != nullptr)
        {
            AddStatesToEnter(*HistoryTargetStates, StateNode.ParentIndex);
        }
        else
        {
            const FTransitionNode& HistoryTransition = Nodes->TransitionNodes[StateNode.TransitionIndex];

            FStateIndexArray TargetStates;
            Algo::Transform(HistoryTransition.Definition->TargetStates, TargetStates, [&](const FGuid& StateID) { return Nodes->StateIDToNodeIndex[StateID]; });
            AddStatesToEnter(TargetStates, StateNode.ParentIndex);
        }
    }
    else
    {
        OutStatesToEnter.AddUnique(StateIndex);

        if (StateNode.Type == EStateType::Compound)
        {
            OutStatesForDefaultEntry.AddUnique(StateIndex);

            FStateIndexArray TargetStates;
            Algo::Transform(Nodes->TransitionNodes[StateNode.InitialTransitionIndex].Definition->TargetStates, TargetStates, [&](const FGuid& StateID) { return Nodes->StateIDToNodeIndex[StateID]; });
            AddStatesToEnter(TargetStates, StateIndex);
        }
        else if(StateNode.Type == EStateType::Parallel)
        {
            ForEachChild(StateIndex, [&](FIndex ChildIndex, auto)
            {
                if (!Algo::AnyOf(OutStatesToEnter, [&](FIndex ExistingIndex) { return IsDescendant(ExistingIndex, ChildIndex); }))
                {
                    AddDescendantStatesToEnter(ChildIndex, OutStatesToEnter, OutStatesForDefaultEntry);
                }
                return true;
            });
        }
    }
}

void FStateChartDefaultExecutor::AddAncestorStatesToEnter(FIndex StateIndex, FIndex AncestorIndex, FStateIndexArray& OutStatesToEnter, FStateIndexArray& OutStatesForDefaultEntry) const
{
    ForEachParent(StateIndex, [&](FIndex Index, const FStateNode& StateNode)
    {
        if (Index == AncestorIndex)
        {
            return false;
        }

        OutStatesToEnter.AddUnique(Index);

        if (StateNode.Type == EStateType::Parallel)
        {
            ForEachChild(StateIndex, [&](FIndex ChildIndex, auto)
            {
                if (!Algo::AnyOf(OutStatesToEnter, [&](FIndex ExistingIndex) { return IsDescendant(ExistingIndex, StateIndex); }))
                {
                    AddDescendantStatesToEnter(ChildIndex, OutStatesToEnter, OutStatesForDefaultEntry);
                }
                return true;
            });
        }

        return true;
    });
}

FIndex FStateChartDefaultExecutor::GetTransitionDomain(const FTransitionNode& Transition) const
{
    FStateIndexArray TargetStates = GetTransitionEffectiveTargets(Transition);

    if (TargetStates.Num() == 0)
    {
        return FIndex::None;
    }

    return FindLeastCommonCompoundAncestor(Transition.SourceNodeIndex, TargetStates);
}

FStateChartDefaultExecutor::FStateIndexArray FStateChartDefaultExecutor::GetTransitionEffectiveTargets(const FTransitionNode& Transition) const
{
    FStateIndexArray Result;

    for (const FGuid& StateID : Transition.Definition->TargetStates)
    {
        FIndex TargetStateIndex = Nodes->StateIDToNodeIndex[StateID];
        const FStateNode& TargetStateNode = Nodes->StateNodes[TargetStateIndex];

        if (TargetStateNode.Type == EStateType::History)
        {
            const FStateIndexArray* HistoryTargetStates = HistoryLookup.Find(TargetStateIndex);
            if (HistoryTargetStates != nullptr)
            {
                AppendUnique(Result, *HistoryTargetStates);
            }
            else
            {
                auto TA = GetTransitionEffectiveTargets(Nodes->TransitionNodes[TargetStateNode.TransitionIndex]);
                AppendUnique(Result, TA);
            }
        }
        else
        {
            Result.Add(TargetStateIndex);
        }
    }

    return Result;
}

FIndex FStateChartDefaultExecutor::FindLeastCommonCompoundAncestor(FIndex BaseIndex, const FStateIndexArray& States) const
{
    FIndex Result;

    ForEachParent(BaseIndex, [&](FIndex Index, const FStateNode& Node)
    {
        if (Algo::AllOf(States, [&](FIndex Index2) { return IsDescendant(Index2, Index); }))
        {
            Result = Index;
            return false;
        }

        return true;
    });

    return Result;
}

bool FStateChartDefaultExecutor::IsDescendant(FIndex Child, FIndex Parent) const
{
    bool bResult = false;
    ForEachParent(Child, [&](FIndex Index, const FStateNode& Node)
    {
        if (Parent == Index)
        {
            bResult = true;
            return false;
        }

        if (Index < Parent)
        {
            // Index is at a higher level than Parent, stop iterating
            return false;
        }

        return true;
    });

    return bResult;
}

void FStateChartDefaultExecutor::ForEachParent(FIndex StateIndex, TFunctionRef<bool(FIndex, const FStateNode&)> Action) const
{
    if (StateIndex.IsNone())
    {
        return;
    }

    FIndex CurrentIndex = Nodes->StateNodes[StateIndex].ParentIndex;
    while (!CurrentIndex.IsNone())
    {
        auto& CurrentNode = Nodes->StateNodes[CurrentIndex];
        if (!Action(CurrentIndex, CurrentNode))
        {
            break;
        }

        CurrentIndex = CurrentNode.ParentIndex;
    }
}

void FStateChartDefaultExecutor::ForEachChild(FIndex StateIndex, TFunctionRef<bool(FIndex, const FStateNode&)> Action) const
{
    if (StateIndex.IsNone())
    {
        return;
    }

    const FStateNode& Node = Nodes->StateNodes[StateIndex];
    for (int32 i = 0; i < Node.NumChildren; ++i)
    {
        const int32 ChildIndex = Node.ChildIndex + i;
        if (!Action(ChildIndex, Nodes->StateNodes[ChildIndex]))
        {
            break;
        }
    }
}

EActionContinuationType FStateChartDefaultExecutor::ExecuteAsyncActionList(const TArray<FInstancedStruct>& ActionList, EActionContinuationType ExistingResult)
{
    for (const FInstancedStruct& ActionStruct : ActionList)
    {
        if (auto* Action = ActionStruct.GetMutablePtr<FStateChartAction>())
        {
            ExistingResult = ExecuteAsyncAction([&] { return Action->ExecuteAsync(Context, CurrentPlan.ContinuationDelegate); }, ExistingResult);
        }
    }

    return ExistingResult;
}

EActionContinuationType FStateChartDefaultExecutor::ExecuteAsyncAction(TFunctionRef<EActionContinuationType()> Action, EActionContinuationType ExistingResult)
{
    TGuardValue<bool> Guard(bInsideActionExecution, true);

    bLastActionExecutedSynchronously = false;
    EActionContinuationType ActionResult = Action();

    if (bLastActionExecutedSynchronously)
    {
        // ignore what was returned because it completed immediately
        ActionResult = EActionContinuationType::Immediate;
    }
    else
    {
        // otherwise remember to wait for this action completion
        CurrentPlan.NumActionsToComplete += 1;
    }

    if (ActionResult == EActionContinuationType::Default)
    {
        // take value from statechart asset
        ActionResult = Asset->GetDefaultContinuationType();
    }

    return FMath::Max(ActionResult, ExistingResult);
}

bool FStateChartDefaultExecutor::EvaluateConditions(const FTransitionNode& TransitionNode, FConstStructView Event) const
{
    for (const FInstancedStruct& Struct : TransitionNode.Definition->Conditions)
    {
        auto* Condition = Struct.GetPtr<FStateChartCondition>();
        if (Condition && !Condition->Evaluate(Context, Event))
        {
            return false;
        }
    }

    return true;
}

}