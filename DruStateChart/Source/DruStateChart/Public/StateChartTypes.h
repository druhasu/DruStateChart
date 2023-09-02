// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartTypes.generated.h"

/* Determines when next Action in the list is executed */
UENUM()
enum class EActionContinuationType : uint8
{
    /* Takes value from StateChart default */
    Default,
    /* As soon as actions in previous list start */
    Immediate,
    /* As soon as first action in previous list finishes */
    FirstFinish,
    /* As soon as all actions in previous list finish */
    LastFinish,
};

/* Determines how History is saved */
UENUM()
enum class EHistoryType : uint8
{
    /* Only direct children state is saved */
    Shallow,
    /* All descendants' states are saved */
    Deep,
};

class IStateChartExecutor;

/*
 * Contains info about executing object and its state
 */
struct FStateChartExecutionContext
{
    FStateChartExecutionContext(IStateChartExecutor& InExecutor, TObjectPtr<UObject> InContextObject)
        : Executor(InExecutor), ContextObject(InContextObject)
    {}

    template <typename T, TEMPLATE_REQUIRES(TIsDerivedFrom<T, UObject>::Value)>
    T* GetContext() const
    {
        return Cast<T>(ContextObject);
    }

    /* Current FStateChartExecutor object */
    IStateChartExecutor& Executor;

    /* Optional object passed from FStateChartExecutor. You may use it to pass any game specific data */
    TObjectPtr<UObject> ContextObject;
};