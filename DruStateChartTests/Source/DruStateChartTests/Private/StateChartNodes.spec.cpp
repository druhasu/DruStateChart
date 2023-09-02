// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Impl/StateChartNodes.h"
#include "Impl/StateChartElements.h"

BEGIN_DEFINE_SPEC(FStateChartNodesSpec, "DruStateChart.StateChart Nodes", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter)

template <typename T>
T* CreateState(const FString& Name, UBaseStateDefinition* Parent = nullptr, TOptional<double> SortOrder = {}) const;

UTransitionDefinition* CreateTransition(UBaseStateDefinition* Source, UBaseStateDefinition* Target, double SortOrder, bool bInitial = false) const;

template <typename T>
void Shuffle(FRandomStream& Stream, TArray<T>& OutArray);

END_DEFINE_SPEC(FStateChartNodesSpec)

void FStateChartNodesSpec::Define()
{
    using namespace StateChart_Impl;

    Describe("Should Create State Hierarchy", [this]
    {
        for (int32 Index = 0; Index < 10; ++Index)
        {
            It("Iteration " + LexToString(Index), [this, Index]
            {
                TArray<TObjectPtr<UBaseStateDefinition>> AllStates;
                AllStates.SetNum(12);

                AllStates[0] = CreateState<UCompoundStateDefinition>("root");
                AllStates[1] = CreateState<UCompoundStateDefinition>("root/a", AllStates[0], 0);
                AllStates[2] = CreateState<UCompoundStateDefinition>("root/b", AllStates[0], 1);
                AllStates[3] = CreateState<UCompoundStateDefinition>("root/a/1", AllStates[1], 0);
                AllStates[4] = CreateState<UCompoundStateDefinition>("root/a/2", AllStates[1], 1);
                AllStates[5] = CreateState<UCompoundStateDefinition>("root/b/1", AllStates[2], 0);
                AllStates[6] = CreateState<UCompoundStateDefinition>("root/b/2", AllStates[2], 1);
                AllStates[7] = CreateState<UCompoundStateDefinition>("root/a/2/1", AllStates[4], 0);
                AllStates[8] = CreateState<UCompoundStateDefinition>("root/a/2/2", AllStates[4], 1);
                AllStates[9] = CreateState<UCompoundStateDefinition>("root/a/2/3", AllStates[4], 2);
                AllStates[10] = CreateState<UCompoundStateDefinition>("root/b/2/1", AllStates[6], 0);
                AllStates[11] = CreateState<UCompoundStateDefinition>("root/b/2/2", AllStates[6], 1);

                auto TempStates = AllStates;

                FRandomStream Stream(Index);
                Shuffle(Stream, TempStates);

                FStateChartNodes Nodes;
                Nodes.CreateNodes(TempStates, {});

                // verify order
                TestEqual("State[0]", Nodes.StateNodes[0].Definition, AllStates[0].Get());
                TestEqual("State[1]", Nodes.StateNodes[1].Definition, AllStates[1].Get());
                TestEqual("State[2]", Nodes.StateNodes[2].Definition, AllStates[2].Get());
                TestEqual("State[3]", Nodes.StateNodes[3].Definition, AllStates[3].Get());
                TestEqual("State[4]", Nodes.StateNodes[4].Definition, AllStates[4].Get());
                TestEqual("State[5]", Nodes.StateNodes[5].Definition, AllStates[5].Get());
                TestEqual("State[6]", Nodes.StateNodes[6].Definition, AllStates[6].Get());
                TestEqual("State[7]", Nodes.StateNodes[7].Definition, AllStates[7].Get());
                TestEqual("State[8]", Nodes.StateNodes[8].Definition, AllStates[8].Get());
                TestEqual("State[9]", Nodes.StateNodes[9].Definition, AllStates[9].Get());
                TestEqual("State[10]", Nodes.StateNodes[10].Definition, AllStates[10].Get());
                TestEqual("State[11]", Nodes.StateNodes[11].Definition, AllStates[11].Get());

                // verify children indexes
                TestEqual("State[0].ChildIndex", Nodes.StateNodes[0].ChildIndex, FIndex(1));
                TestEqual("State[1].ChildIndex", Nodes.StateNodes[1].ChildIndex, FIndex(3));
                TestEqual("State[2].ChildIndex", Nodes.StateNodes[2].ChildIndex, FIndex(5));
                TestEqual("State[3].ChildIndex", Nodes.StateNodes[3].ChildIndex, FIndex(0));
                TestEqual("State[4].ChildIndex", Nodes.StateNodes[4].ChildIndex, FIndex(7));
                TestEqual("State[5].ChildIndex", Nodes.StateNodes[5].ChildIndex, FIndex(0));
                TestEqual("State[6].ChildIndex", Nodes.StateNodes[6].ChildIndex, FIndex(10));
                TestEqual("State[7].ChildIndex", Nodes.StateNodes[7].ChildIndex, FIndex(0));
                TestEqual("State[8].ChildIndex", Nodes.StateNodes[8].ChildIndex, FIndex(0));
                TestEqual("State[9].ChildIndex", Nodes.StateNodes[9].ChildIndex, FIndex(0));
                TestEqual("State[10].ChildIndex", Nodes.StateNodes[10].ChildIndex, FIndex(0));
                TestEqual("State[11].ChildIndex", Nodes.StateNodes[11].ChildIndex, FIndex(0));

                // verify children counts
                TestEqual("State[0].NumChildren", Nodes.StateNodes[0].NumChildren, 2);
                TestEqual("State[1].NumChildren", Nodes.StateNodes[1].NumChildren, 2);
                TestEqual("State[2].NumChildren", Nodes.StateNodes[2].NumChildren, 2);
                TestEqual("State[3].NumChildren", Nodes.StateNodes[3].NumChildren, 0);
                TestEqual("State[4].NumChildren", Nodes.StateNodes[4].NumChildren, 3);
                TestEqual("State[5].NumChildren", Nodes.StateNodes[5].NumChildren, 0);
                TestEqual("State[6].NumChildren", Nodes.StateNodes[6].NumChildren, 2);
                TestEqual("State[7].NumChildren", Nodes.StateNodes[7].NumChildren, 0);
                TestEqual("State[8].NumChildren", Nodes.StateNodes[8].NumChildren, 0);
                TestEqual("State[9].NumChildren", Nodes.StateNodes[9].NumChildren, 0);
                TestEqual("State[10].NumChildren", Nodes.StateNodes[10].NumChildren, 0);
                TestEqual("State[11].NumChildren", Nodes.StateNodes[11].NumChildren, 0);
            });
        }
    });

    Describe("Should Create Transition Nodes", [this]
    {
        for (int32 Index = 0; Index < 10; ++Index)
        {
            It("Iteration " + LexToString(Index), [this, Index]
            {
                TArray<TObjectPtr<UBaseStateDefinition>> AllStates;
                AllStates.SetNum(4);

                AllStates[0] = CreateState<UCompoundStateDefinition>("root");
                AllStates[1] = CreateState<UCompoundStateDefinition>("root/a", AllStates[0], 0);
                AllStates[2] = CreateState<UCompoundStateDefinition>("root/b", AllStates[0], 1);
                AllStates[3] = CreateState<UCompoundStateDefinition>("root/c", AllStates[0], 2);

                TArray<TObjectPtr<UTransitionDefinition>> AllTransitions;
                AllTransitions.SetNum(8);

                AllTransitions[0] = CreateTransition(AllStates[0], AllStates[1], 0);
                AllTransitions[1] = CreateTransition(AllStates[0], AllStates[2], 1);
                AllTransitions[2] = CreateTransition(AllStates[1], AllStates[0], 0);
                AllTransitions[3] = CreateTransition(AllStates[1], AllStates[2], 1);
                AllTransitions[4] = CreateTransition(AllStates[1], AllStates[3], 2);
                AllTransitions[5] = CreateTransition(AllStates[2], AllStates[0], 0);
                AllTransitions[6] = CreateTransition(AllStates[2], AllStates[1], 1);
                AllTransitions[7] = CreateTransition(AllStates[3], AllStates[2], 0);

                auto TempTransitions = AllTransitions;

                FRandomStream Stream(Index);
                Shuffle(Stream, TempTransitions);

                FStateChartNodes Nodes;
                Nodes.CreateNodes(AllStates, TempTransitions);

                // verify order
                TestEqual("Transition[0]", Nodes.TransitionNodes[0].Definition, AllTransitions[0].Get());
                TestEqual("Transition[1]", Nodes.TransitionNodes[1].Definition, AllTransitions[1].Get());
                TestEqual("Transition[2]", Nodes.TransitionNodes[2].Definition, AllTransitions[2].Get());
                TestEqual("Transition[3]", Nodes.TransitionNodes[3].Definition, AllTransitions[3].Get());
                TestEqual("Transition[4]", Nodes.TransitionNodes[4].Definition, AllTransitions[4].Get());
                TestEqual("Transition[5]", Nodes.TransitionNodes[5].Definition, AllTransitions[5].Get());
                TestEqual("Transition[6]", Nodes.TransitionNodes[6].Definition, AllTransitions[6].Get());
                TestEqual("Transition[7]", Nodes.TransitionNodes[7].Definition, AllTransitions[7].Get());

                // verify indexes
                TestEqual("State[0].TransitionIndex", Nodes.StateNodes[0].TransitionIndex, FIndex(0));
                TestEqual("State[1].TransitionIndex", Nodes.StateNodes[1].TransitionIndex, FIndex(2));
                TestEqual("State[2].TransitionIndex", Nodes.StateNodes[2].TransitionIndex, FIndex(5));
                TestEqual("State[3].TransitionIndex", Nodes.StateNodes[3].TransitionIndex, FIndex(7));

                // verify counts
                TestEqual("State[0].NumTransitions", Nodes.StateNodes[0].NumTransitions, 2);
                TestEqual("State[1].NumTransitions", Nodes.StateNodes[1].NumTransitions, 3);
                TestEqual("State[2].NumTransitions", Nodes.StateNodes[2].NumTransitions, 2);
                TestEqual("State[3].NumTransitions", Nodes.StateNodes[3].NumTransitions, 1);
            });
        }
    });

    It("Should Create Initial Transition Node", [this]
    {
        TArray<TObjectPtr<UBaseStateDefinition>> AllStates;
        AllStates.SetNum(4);

        AllStates[0] = CreateState<UCompoundStateDefinition>("root");
        AllStates[1] = CreateState<UCompoundStateDefinition>("root/a", AllStates[0], 0);
        AllStates[2] = CreateState<UCompoundStateDefinition>("root/b", AllStates[0], 1);
        AllStates[3] = CreateState<UCompoundStateDefinition>("root/c", AllStates[0], 2);

        TArray<TObjectPtr<UTransitionDefinition>> AllTransitions;
        AllTransitions.SetNum(3);

        AllTransitions[0] = CreateTransition(AllStates[0], AllStates[1], 0);
        AllTransitions[1] = CreateTransition(AllStates[0], AllStates[2], 1);
        AllTransitions[2] = CreateTransition(AllStates[0], AllStates[3], -1, true);

        auto TempTransitions = AllTransitions;

        FRandomStream Stream(123);
        Shuffle(Stream, TempTransitions);

        FStateChartNodes Nodes;
        Nodes.CreateNodes(AllStates, TempTransitions);

        // verify order
        TestEqual("Transition[0]", Nodes.TransitionNodes[0].Definition, AllTransitions[0].Get());
        TestEqual("Transition[1]", Nodes.TransitionNodes[1].Definition, AllTransitions[1].Get());
        TestEqual("Transition[2]", Nodes.TransitionNodes[2].Definition, AllTransitions[2].Get());

        // verify indexes and counter
        TestEqual("TransitionIndex", Nodes.StateNodes[0].TransitionIndex, 0);
        TestEqual("NumTransitions", Nodes.StateNodes[0].NumTransitions, 2); // initial transition is not counted here
        TestEqual("InitialTransitionIndex", Nodes.StateNodes[0].InitialTransitionIndex, 2);
    });
}

template <typename T>
T* FStateChartNodesSpec::CreateState(const FString& Name, UBaseStateDefinition* Parent, TOptional<double> SortOrder) const
{
    auto State = NewObject<T>();
    State->FriendlyName = Name;

    if (Parent)
    {
        State->ParentID = Parent->ID;
    }

    auto SortableState = Cast<UActivatableStateDefinition>(State);
    if (SortOrder.IsSet() && SortableState)
    {
        SortableState->SortOrder = SortOrder.GetValue();
    }

    return State;
}

UTransitionDefinition* FStateChartNodesSpec::CreateTransition(UBaseStateDefinition* Source, UBaseStateDefinition* Target, double SortOrder, bool bInitial) const
{
    UTransitionDefinition* Transition = NewObject<UTransitionDefinition>();
    Transition->SourceState = Source->ID;
    Transition->SortOrder = SortOrder;
    Transition->bInitial = bInitial;

    Transition->TargetStates.Add(Target->ID);

    return Transition;
}

template <typename T>
void FStateChartNodesSpec::Shuffle(FRandomStream& Stream, TArray<T>& OutArray)
{
    if (OutArray.Num() > 1)
    {
        for (int32 i = 0; i < OutArray.Num(); i++)
        {
            const int32 j = FMath::RandRange(i, OutArray.Num() - 1);
            if (i != j)
            {
                OutArray.Swap(i, j);
            }
        }
    }
}