// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartTypes.h"
#include "Delegates/Delegate.h"
#include "StructView.h"
#include "StateHandler.generated.h"

/*
 * Handler object of a state.
 * Instance of this object is created when state is entered and discarded after state is exited.
 * You can use it to perform some work while state is active
 * IMPORTANT: If you subscribed to any delegates, you must unsubscribe from them inside overriden StateExited/StateExitedAsync, because this object may be pooled
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class DRUSTATECHART_API UStateHandler : public UObject
{
    GENERATED_BODY()

public:

    /*
     * Called when state is entered before any other action.
     * Override this method if your Object needs asynchronous execution.
     * Return value tells executor when next action in the list should be executed.
     * Done must be called when Object finishes with its work.
     * 
     * 'Event' parameter contains event data that triggered transition. It may be empty, if transition was not triggered by event
     */
    virtual EActionContinuationType StateEnteredAsync(FConstStructView Event, const FStateChartExecutionContext& Context, const FSimpleDelegate& Done)
    {
        return StateEntered(Event, Context), EActionContinuationType::Immediate;
    }

    /*
     * Called when state is entered before any other action.
     * Override this method if your Object does not require asynchronous execution.
     * 
     * 'Event' parameter contains event data that triggered transition. It may be empty, if transition was not triggered by event
     */
    virtual void StateEntered(FConstStructView Event, const FStateChartExecutionContext& Context) {}

    /*
     * Called when state is exited after all other actions.
     * Override this method if your Object needs asynchronous execution.
     * Return value tells executor when next action in the list should be executed.
     * Done must be called when Object finishes with its work.
     */
    virtual EActionContinuationType StateExitedAsync(const FStateChartExecutionContext& Context, const FSimpleDelegate& Done)
    {
        return StateExited(Context), EActionContinuationType::Immediate;
    }

    /*
     * Called when state is exited after all other actions.
     * Override this method if your Object does not require asynchronous execution.
     */
    virtual void StateExited(const FStateChartExecutionContext& Context) {}
};