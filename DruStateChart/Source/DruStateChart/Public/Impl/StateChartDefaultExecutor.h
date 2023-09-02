// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "UObject/GCObject.h"
#include "Interfaces/IStateChartExecutor.h"
#include "StateChartTypes.h"
#include "Impl/StateChartNodes.h"
#include "UObject/ObjectPtr.h"
#include "Containers/RingBuffer.h"
#include "InstancedStruct.h"
#include "StructView.h"
#include "Concepts/StaticStructProvider.h"

namespace DruStateChart_Impl
{

/*
 * Default implementation of an executor
 */
class DRUSTATECHART_API FStateChartDefaultExecutor : public FGCObject, public IStateChartExecutor
{
public:
    using FHandlerCreated = TMulticastDelegate<void(UStateHandler& NewHandler)>;

    FStateChartDefaultExecutor(UStateChartAsset& StateChartAsset, TObjectPtr<UObject> ContextObject = nullptr);

    TObjectPtr<UStateChartAsset> GetExecutingAsset() const override { return Asset; }
    void Execute() override;
    TArray<TObjectPtr<UBaseStateDefinition>> GetActiveStates() const override;
    FHandlerCreated& OnStateHandlerCreated() override { return StateHandlerCreatedDelegate; }

    // Begin FGCObject overrides
    void AddReferencedObjects(FReferenceCollector& Collector) override;
    FString GetReferencerName() const override;
    //~End FGCObject overrides

private:
    class FStateIndexArray : public TArray<FIndex, TInlineAllocator<24>> {};
    class FTransitionIndexArray : public TArray<FIndex, TInlineAllocator<24>> {};

    enum class EStepType : uint8
    {
        Exit, Transition, Enter
    };

    struct FExecutionPlanStep
    {
        EStepType Type;
        FIndex ObjectIndex;
    };

    struct FExecutionPlan
    {
        FExecutionPlan() = default;
        FExecutionPlan(uint16 NewIndex) : PlanIndex(NewIndex) {}

        uint16 PlanIndex = 0;
        uint16 StepIndex = 0;
        uint16 NumActionsToComplete = 0;
        EActionContinuationType ContinuationType = EActionContinuationType::Default;

        TArray<FExecutionPlanStep, TInlineAllocator<32>> Steps;
        FStateIndexArray StatesForDefaultEntry;

        FSimpleDelegate ContinuationDelegate;
        FInstancedStruct Event;
    };

    void ExecuteEventImpl(FConstStructView Event) override;
    void StartNewPlan(const FTransitionIndexArray& Transitions, FInstancedStruct Event);

    void ProcessEventsSynchronous();
    void ProcessPlanSynchronous();

    void RecordHistoryStates(const FStateIndexArray& StatesToExit);

    EActionContinuationType ExitStateAsync(FIndex NodeIndex);
    EActionContinuationType ExecuteTransitionActionsAsync(FIndex TransitionIndex);
    EActionContinuationType EnterStateAsync(FIndex NodeIndex);

    void OnActionCompleted(uint16 PlanIndex, uint16 StepIndex);

    FTransitionIndexArray CollectTransitions(FConstStructView Event);
    FTransitionIndexArray RemoveConflictingTransitions(const FTransitionIndexArray& Transitions);

    void CollectStatesToExit(const FTransitionIndexArray& Transitions, FStateIndexArray& OutStatesToExit) const;
    void CollectStatesToEnter(const FTransitionIndexArray& Transitions, FStateIndexArray& OutStatesToEnter, FStateIndexArray& OutStatesForDefaultEntry) const;
    void AddDescendantStatesToEnter(FIndex StateIndex, FStateIndexArray& OutStatesToEnter, FStateIndexArray& OutStatesForDefaultEntry) const;
    void AddAncestorStatesToEnter(FIndex StateIndex, FIndex AncestorIndex, FStateIndexArray& OutStatesToEnter, FStateIndexArray& OutStatesForDefaultEntry) const;

    FIndex GetTransitionDomain(const FTransitionNode& Transition) const;
    FStateIndexArray GetTransitionEffectiveTargets(const FTransitionNode& Transition) const;
    FIndex FindLeastCommonCompoundAncestor(FIndex BaseIndex, const FStateIndexArray& States) const;
    bool IsDescendant(FIndex Child, FIndex Parent) const;

    void ForEachParent(FIndex StateIndex, TFunctionRef<bool(FIndex, const FStateNode&)> Action) const;
    void ForEachChild(FIndex StateIndex, TFunctionRef<bool(FIndex, const FStateNode&)> Action) const;

    EActionContinuationType ExecuteAsyncActionList(TArray<FInstancedStruct>& ActionList, EActionContinuationType ExistingResult);
    EActionContinuationType ExecuteAsyncAction(TFunctionRef<EActionContinuationType()> Action, EActionContinuationType ExistingResult);
    bool EvaluateConditions(const FTransitionNode& TransitionNode, FConstStructView Event) const;

    TObjectPtr<UStateChartAsset> Asset;
    FStateChartExecutionContext Context;
    FHandlerCreated StateHandlerCreatedDelegate;

    const FStateChartNodes* Nodes;

    // all active states
    TArray<FIndex> ActiveStates;

    TMap<FIndex, FStateIndexArray> HistoryLookup;
    TMultiMap<FIndex, TObjectPtr<UStateHandler>> StateHandlers;

    TRingBuffer<FInstancedStruct, TInlineAllocator<8>> ExternalEventQueue;
    TRingBuffer<FInstancedStruct, TInlineAllocator<8>> InternalEventQueue;

    FExecutionPlan CurrentPlan;
    bool bExecutingPlan = false;

    bool bInsideExecutionLoop = false;
    bool bLastActionExecutedSynchronously = false;
    bool bInsideActionExecution = false;
};

}