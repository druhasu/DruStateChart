// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Impl/StateChartNodes.h"
#include "Impl/StateChartElements.h"

namespace StateChart_Impl
{

const FIndex FIndex::None = 0xffff;

void FStateChartNodes::CreateNodes(const TArray<TObjectPtr<UBaseStateDefinition>>& States, const TArray<TObjectPtr<UTransitionDefinition>>& Transitions)
{
    StateNodes.Empty(States.Num());
    TransitionNodes.Empty(Transitions.Num());

    StateIDToNodeIndex.Empty(States.Num());
    StateIDToDefinition.Empty(States.Num());

    CreateStateNodes(States);
    SortStateNodes();
    UpdateHierarchyReferences();
    MarkAtomicStates();

    CreateTransitionNodes(Transitions);
    SortTransitionNodes();
    UpdateTransitions();
}

void FStateChartNodes::CreateStateNodes(const TArray<TObjectPtr<UBaseStateDefinition>>& States)
{
    // create nodes and add them to array
    for (int32 StateIndex = 0; StateIndex < States.Num(); ++StateIndex)
    {
        auto State = States[StateIndex];

        int32 NewNodeIdx = StateNodes.Emplace(GetStateType(State), State);

        StateIDToDefinition.Emplace(State->ID, State);
    }
}

void FStateChartNodes::SortStateNodes()
{
    // sort nodes by hierarchy and sort order
    StateNodes.Sort([&](const FStateNode& A, const FStateNode& B)
    {
        return CompareStates(A.Definition, B.Definition);
    });
}

void FStateChartNodes::UpdateHierarchyReferences()
{
    // update parent/child references and child count
    for (int32 StateIndex = 0; StateIndex < StateNodes.Num(); ++StateIndex)
    {
        auto& Node = StateNodes[StateIndex];
        StateIDToNodeIndex.Emplace(Node.Definition->ID, FIndex(StateIndex));

        FIndex ParentIndex = StateIDToNodeIndex.FindRef(Node.Definition->ParentID);
        if (!ParentIndex.IsNone())
        {
            Node.ParentIndex = ParentIndex;

            auto& ParentNode = StateNodes[ParentIndex];
            if (ParentNode.ChildIndex == FIndex(0))
            {
                ParentNode.ChildIndex = StateIndex;
            }
            ParentNode.NumChildren++;
        }
    }
}

void FStateChartNodes::MarkAtomicStates()
{
    // mark compound states without children as atomic
    for (int32 StateIndex = 0; StateIndex < StateNodes.Num(); ++StateIndex)
    {
        auto& Node = StateNodes[StateIndex];
        if (Node.Type == EStateType::Compound && Node.NumChildren == 0)
        {
            Node.Type = EStateType::Atomic;
        }
    }
}

void FStateChartNodes::CreateTransitionNodes(const TArray<TObjectPtr<UTransitionDefinition>>& Transitions)
{
    TransitionNodes.Empty(Transitions.Num());

    for (int32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); ++TransitionIndex)
    {
        auto Transition = Transitions[TransitionIndex];

        FIndex SourceStateIdx = StateIDToNodeIndex.FindRef(Transition->SourceState);

        if (ensure(!SourceStateIdx.IsNone()))
        {
            TransitionNodes.Emplace(SourceStateIdx, Transition->EventID, Transition);
        }
    }
}

void FStateChartNodes::SortTransitionNodes()
{
    // sort transitions by their source index and sort order
    TransitionNodes.Sort([](const FTransitionNode& A, const FTransitionNode& B)
    {
        if (A.SourceNodeIndex < B.SourceNodeIndex)
        {
            return true;
        }

        if (A.SourceNodeIndex == B.SourceNodeIndex)
        {
            if (!A.Definition->bInitial && B.Definition->bInitial)
            {
                // initial transition goes last
                return true;
            }
            
            if (A.Definition->bInitial == B.Definition->bInitial)
            {
                return A.Definition->SortOrder < B.Definition->SortOrder;
            }
        }

        return false;
    });
}

void FStateChartNodes::UpdateTransitions()
{
    // update transition indexes and count inside state nodes
    int32 TransitionIndex = 0;
    for (int32 StateIndex = 0; StateIndex < StateNodes.Num(); ++StateIndex)
    {
        auto& Node = StateNodes[StateIndex];
        Node.TransitionIndex = TransitionIndex;

        // count transitions
        while (TransitionNodes.IsValidIndex(TransitionIndex) && TransitionNodes[TransitionIndex].SourceNodeIndex == FIndex(StateIndex))
        {
            if (!TransitionNodes[TransitionIndex].Definition->bInitial)
            {
                Node.NumTransitions++;
            }
            else
            {
                Node.InitialTransitionIndex = TransitionIndex;
            }
            TransitionIndex++;
        }
    }
}

EStateType FStateChartNodes::GetStateType(TObjectPtr<UBaseStateDefinition> Definition) const
{
    static TMap<UClass*, EStateType, TFixedSetAllocator<4>> Map
    {
        { UHistoryStateDefinition::StaticClass(),  EStateType::History  },
        { UFinalStateDefinition::StaticClass(),    EStateType::Final    },
        { UCompoundStateDefinition::StaticClass(), EStateType::Compound },
        { UParallelStateDefinition::StaticClass(), EStateType::Parallel },
    };

    return Map.FindRef(Definition->GetClass());
}

bool FStateChartNodes::CompareStates(UBaseStateDefinition* AState, UBaseStateDefinition* BState) const
{
    if (!AState->ParentID.IsValid() || AState->ID == BState->ParentID)
    {
        return true;
    }

    if (AState->ParentID == BState->ParentID)
    {
        auto ASortable = Cast<UActivatableStateDefinition>(AState);
        auto BSortable = Cast<UActivatableStateDefinition>(BState);

        if (ASortable && !BSortable)
        {
            return true;
        }

        if (ASortable && BSortable)
        {
            return ASortable->SortOrder < BSortable->SortOrder;
        }
    }

    auto AParent = StateIDToDefinition.FindRef(AState->ParentID);
    auto BParent = StateIDToDefinition.FindRef(BState->ParentID);
    if (AParent && BParent)
    {
        return CompareStates(AParent, BParent);
    }

    return false;
}

}