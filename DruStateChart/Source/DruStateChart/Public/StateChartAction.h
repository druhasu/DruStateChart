// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartTypes.h"
#include "Delegates/Delegate.h"
#include "UObject/ObjectPtr.h"
#include "StateChartAction.generated.h"

/*
 * Action performed by StateChart when entering or exiting the state.
 * Override either ExecuteAsync or Execute, not both
 */
USTRUCT()
struct DRUSTATECHART_API FStateChartAction
{
    GENERATED_BODY()

public:
    virtual ~FStateChartAction() = default;

    /*
     * Override this method if your Action needs asynchronous execution.
     * Return value tells executor when next action in the list should be executed.
     * Done must be called when action finishes with its work.
     * 
     * Context contains info about executing object
     */
    virtual EActionContinuationType ExecuteAsync(const FStateChartExecutionContext& Context, const FSimpleDelegate& Done)
    {
        return Execute(Context), EActionContinuationType::Immediate;
    }

    /*
     * Override this method if your Action does not require asynchronous execution.
     * 
     * Context contains info about executing object
     */
    virtual void Execute(const FStateChartExecutionContext& Context) {}
};