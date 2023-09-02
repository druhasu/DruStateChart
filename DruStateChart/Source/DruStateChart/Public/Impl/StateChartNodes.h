// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"

class UBaseStateDefinition;
class UTransitionDefinition;

namespace StateChart_Impl
{
    enum class EStateType : uint8
    {
        Compound,
        Parallel,
        Atomic,
        Final,
        History,
    };

    struct DRUSTATECHART_API FIndex
    {
        static const FIndex None;

        FIndex() : Value(None) {}
        FIndex(uint16 InValue) : Value(InValue) {}
        FIndex(int32 InValue) : Value(InValue < 0 ? 0xffff : static_cast<uint16>(InValue)) {}

        bool IsNone() const
        {
            return *this == None;
        }

        friend bool operator== (FIndex A, FIndex B)
        {
            return A.Value == B.Value;
        }

        friend bool operator!= (FIndex A, FIndex B)
        {
            return A.Value != B.Value;
        }

        friend bool operator< (FIndex A, FIndex B)
        {
            return A.Value < B.Value;
        }

        operator int32() const
        {
            return *this == None ? INDEX_NONE : Value;
        }

    private:
        uint16 Value;
    };

    struct DRUSTATECHART_API FStateNode
    {
        FStateNode(EStateType InType, UBaseStateDefinition* InDefinition)
            : Type(InType)
            , ParentIndex(FIndex::None)
            , TransitionIndex(0)
            , NumTransitions(0)
            , InitialTransitionIndex(FIndex::None)
            , ChildIndex(0)
            , NumChildren(0)
            , Definition(InDefinition)
        {}

        template <typename T>
        T* GetDefinition() const
        {
            return (T*)Definition;
        }

        EStateType Type;
        FIndex ParentIndex;

        FIndex TransitionIndex;
        uint16 NumTransitions;
        FIndex InitialTransitionIndex; // only for Compound and History states

        FIndex ChildIndex;
        uint16 NumChildren;

        UBaseStateDefinition* Definition;
    };

    struct DRUSTATECHART_API FTransitionNode
    {
        FTransitionNode(FIndex InSourceNodeIndex, UScriptStruct* InEventID, UTransitionDefinition* InDefinition)
            : SourceNodeIndex(InSourceNodeIndex)
            , EventID(InEventID)
            , Definition(InDefinition)
        {}

        FIndex SourceNodeIndex;

        UScriptStruct* EventID;
        UTransitionDefinition* Definition;
    };

    struct DRUSTATECHART_API FStateChartNodes
    {
        void CreateNodes(const TArray<TObjectPtr<UBaseStateDefinition>>& States, const TArray<TObjectPtr<UTransitionDefinition>>& Transitions);

        TArray<FStateNode> StateNodes;
        TArray<FTransitionNode> TransitionNodes;

        TMap<FGuid, FIndex> StateIDToNodeIndex;
        TMap<FGuid, TObjectPtr<UBaseStateDefinition>> StateIDToDefinition;

    private:
        void CreateStateNodes(const TArray<TObjectPtr<UBaseStateDefinition>>& States);
        void SortStateNodes();
        void UpdateHierarchyReferences();
        void MarkAtomicStates();

        void CreateTransitionNodes(const TArray<TObjectPtr<UTransitionDefinition>>& Transitions);
        void SortTransitionNodes();
        void UpdateTransitions();

        EStateType GetStateType(TObjectPtr<UBaseStateDefinition> Definition) const;
        bool CompareStates(UBaseStateDefinition* AState, UBaseStateDefinition* BState) const;
    };
}