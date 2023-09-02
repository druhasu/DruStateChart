// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartTypes.h"
#include "StateChartAction.h"
#include "StateChartCondition.h"
#include "StateChartEvent.h"
#include "InstancedStruct.h"
#include "UObject/ObjectPtr.h"

class FStateChartBuilder;
class UStateHandler;

namespace DruStateChart_Impl
{
    struct FTransitionBuilder;

    struct FBuilderBase
    {
        FBuilderBase(FString InName) : Name(MoveTemp(InName)) {}

    protected:
        template <typename T> friend struct TTransitionOp;
        template <typename T, bool> friend struct TChildrenOp;
        friend class ::FStateChartBuilder;

        FString Name;
        FBuilderBase* Parent = nullptr;
        double SortOrder = 0;
    };

    template<typename T>
    struct TInitialStateOp
    {
        /* Set path to initial state */
        T& Initial(FString InStatePath)
        {
            InitialStatePath = MoveTemp(InStatePath);
            return *static_cast<T*>(this);
        }

    protected:
        FString InitialStatePath;
    };

    template<typename T>
    struct TOnEnterOp
    {
        /* Adds an Action to be Executed when state is entered */
        template <typename TAction, TEMPLATE_REQUIRES(TIsDerivedFrom<TAction, FStateChartAction>::Value)>
        T& OnEnter(TAction Action)
        {
            EnterActions.Emplace(FInstancedStruct::Make(MoveTemp(Action)));
            return *static_cast<T*>(this);
        }

        /* Adds an Action to be Executed when state is entered */
        T& OnEnter(FInstancedStruct Action)
        {
            check(Action.IsValid());
            check(Action.GetScriptStruct()->IsChildOf(FStateChartAction::StaticStruct()));

            EnterActions.Emplace(MoveTemp(Action));
            return *static_cast<T*>(this);
        }

    protected:
        TArray<FInstancedStruct> EnterActions;
    };

    template<typename T>
    struct TOnExitOp
    {
        /* Adds an Action to be Executed when state is exited */
        template <typename TAction, TEMPLATE_REQUIRES(TIsDerivedFrom<TAction, FStateChartAction>::Value)>
        T& OnExit(TAction Action)
        {
            ExitActions.Emplace(FInstancedStruct::Make(MoveTemp(Action)));
            return *static_cast<T*>(this);
        }

        /* Adds an Action to be Executed when state is exited */
        T& OnExit(FInstancedStruct Action)
        {
            check(Action.IsValid());
            check(Action.GetScriptStruct()->IsChildOf(FStateChartAction::StaticStruct()));

            ExitActions.Emplace(MoveTemp(Action));
            return *static_cast<T*>(this);
        }

    protected:
        TArray<FInstancedStruct> ExitActions;
    };

    template<typename T>
    struct THandlerOp
    {
        /* Adds StateHadnler */
        template <typename THandler, TEMPLATE_REQUIRES(TIsDerivedFrom<THandler, UStateHandler>::Value)>
        T& Handler()
        {
            return Handler(NewObject<THandler>());
        }

        /* Adds StateHadnler */
        T& Handler(TObjectPtr<UStateHandler> Handler)
        {
            check(Handler != nullptr);

            Handlers.Emplace(Handler);
            return *static_cast<T*>(this);
        }

    protected:
        TArray<TObjectPtr<UStateHandler>> Handlers;
    };

    template<typename T>
    struct THistoryTypeOp
    {
        /* Set History type */
        T& HistoryType(EHistoryType InType)
        {
            Type = InType;
            return *static_cast<T*>(this);
        }

    protected:
        EHistoryType Type;
    };

    template<typename T, bool bAllowTransitions>
    struct TChildrenOp
    {
        /* Adds Children states and transition */
        template <typename... TChildren>
        T& Children(TChildren&... InChildren)
        {
            int32 Index = 0;
            (AddChildBuilder(InChildren, Index++), ...);
            return *static_cast<T*>(this);
        }

    protected:
        template <typename TBuilder>
        void AddChildBuilder(TBuilder& Builder, int32 Index)
        {
            static_assert(!std::is_same_v<TBuilder, FTransitionBuilder> || bAllowTransitions, "Transition children are not allowed");

            Builder.Parent = static_cast<T*>(this);
            Builder.SortOrder = Index;
        }
    };

    template<typename T>
    struct TInitialTransitionOp
    {
        /* Marks transition as initial */
        T& Initial()
        {
            bInitial = true;
            return *static_cast<T*>(this);
        }

    protected:
        bool bInitial = false;
    };

    template<typename T>
    struct TTargetOp
    {
        /* Sets target state */
        T& Target(FString InStatePath)
        {
            TargetPaths.Emplace(MoveTemp(InStatePath));
            return *static_cast<T*>(this);
        }

    protected:
        TArray<FString> TargetPaths;
    };

    template<typename T>
    struct TEventOp
    {
        /* Sets event that will trigger transition */
        template <typename TEvent>
        T& Event()
        {
            EventStruct = TEvent::StaticStruct();
            return *static_cast<T*>(this);
        }

        /* Sets event that will trigger transition */
        T& Event(TObjectPtr<UScriptStruct> InEventStruct)
        {
            check(InEventStruct);
            EventStruct = InEventStruct;
            return *static_cast<T*>(this);
        }

    protected:
        TObjectPtr<UScriptStruct> EventStruct;
    };

    template <typename T>
    struct TEventTagOp
    {
        /* Set this transition to trigger upon receiving event with given GameplayTag */
        T& EventTag(FName EventTagName, bool bMatchExact = false)
        {
            return EventTag(FGameplayTag::RequestGameplayTag(EventTagName), bMatchExact);
        }

        /* Set this transition to trigger upon receiving event with given GameplayTag */
        T& EventTag(const FGameplayTag& EventTag, bool bMatchExact = false)
        {
            check(EventTag.IsValid());

            T& CastedThis = *static_cast<T*>(this);
            CastedThis.Event<FStateChartGenericEvent>();
            CastedThis.Condition(FStateChartGenericEventCondition(EventTag, bMatchExact));
            return CastedThis;
        }
    };

    template<typename T>
    struct TConditionOp
    {
        /* Sets condition of transition */
        template <typename TCondition, TEMPLATE_REQUIRES(TIsDerivedFrom<TCondition, FStateChartCondition>::Value)>
        T& Condition(TCondition&& Condition)
        {
            Conditions.Emplace(FInstancedStruct::Make(MoveTemp(Condition)));
            return *static_cast<T*>(this);
        }

        /* Sets condition of transition */
        T& Condition(FInstancedStruct&& Condition)
        {
            check(Condition.IsValid());
            check(Condition.GetScriptStruct()->IsChildOf(FStateChartCondition::StaticStruct()));

            Conditions.Emplace(MoveTemp(Condition));
            return *static_cast<T*>(this);
        }

    protected:
        TArray<FInstancedStruct> Conditions;
    };

    template<typename T>
    struct TActionOp
    {
        /* Adds Action to be Executed when transition is taken */
        template <typename TAction, TEMPLATE_REQUIRES(TIsDerivedFrom<TAction, FStateChartAction>::Value)>
        T& Action(TAction Action)
        {
            Actions.Emplace(FInstancedStruct::Make(MoveTemp(Action)));
            return *static_cast<T*>(this);
        }

        /* Adds Action to be Executed when transition is taken */
        T& Action(FInstancedStruct Action)
        {
            check(Action.IsValid());
            check(Action.GetScriptStruct()->IsChildOf(FStateChartAction::StaticStruct()));

            Actions.Emplace(MoveTemp(Action));
            return *static_cast<T*>(this);
        }

    protected:
        TArray<FInstancedStruct> Actions;
    };

    struct FTransitionBuilder
        : public DruStateChart_Impl::FBuilderBase
        , public DruStateChart_Impl::TInitialTransitionOp<FTransitionBuilder>
        , public DruStateChart_Impl::TEventOp<FTransitionBuilder>
        , public DruStateChart_Impl::TEventTagOp<FTransitionBuilder>
        , public DruStateChart_Impl::TTargetOp<FTransitionBuilder>
        , public DruStateChart_Impl::TConditionOp<FTransitionBuilder>
        , public DruStateChart_Impl::TActionOp<FTransitionBuilder>
    {
        FTransitionBuilder() : DruStateChart_Impl::FBuilderBase(FString()) {}
        friend class ::FStateChartBuilder;
    };

    struct FStateBuilder
        : public DruStateChart_Impl::FBuilderBase
        , public DruStateChart_Impl::TInitialStateOp<FStateBuilder>
        , public DruStateChart_Impl::TOnEnterOp<FStateBuilder>
        , public DruStateChart_Impl::TOnExitOp<FStateBuilder>
        , public DruStateChart_Impl::THandlerOp<FStateBuilder>
        , public DruStateChart_Impl::TChildrenOp<FStateBuilder, true>
    {
        FStateBuilder(FString Name) : DruStateChart_Impl::FBuilderBase(Name) {}
        friend class ::FStateChartBuilder;
    };

    struct FParallelBuilder
        : public DruStateChart_Impl::FBuilderBase
        , public DruStateChart_Impl::TOnEnterOp<FParallelBuilder>
        , public DruStateChart_Impl::TOnExitOp<FParallelBuilder>
        , public DruStateChart_Impl::THandlerOp<FParallelBuilder>
        , public DruStateChart_Impl::TChildrenOp<FParallelBuilder, true>
    {
        FParallelBuilder(FString Name) : DruStateChart_Impl::FBuilderBase(Name) {}
        friend class ::FStateChartBuilder;
    };

    struct FHistoryBuilder
        : public DruStateChart_Impl::FBuilderBase
        , public DruStateChart_Impl::TInitialStateOp<FHistoryBuilder>
        , public DruStateChart_Impl::THistoryTypeOp<FHistoryBuilder>
    {
        FHistoryBuilder(FString Name) : DruStateChart_Impl::FBuilderBase(Name) {}
        friend class ::FStateChartBuilder;
    };

    struct FRootBuilder
        : public DruStateChart_Impl::FBuilderBase
        , public DruStateChart_Impl::TInitialStateOp<FRootBuilder>
        , public DruStateChart_Impl::TChildrenOp<FRootBuilder, false>
    {
        FRootBuilder() : DruStateChart_Impl::FBuilderBase(TEXT("root")) {}
        friend class ::FStateChartBuilder;
    };
}