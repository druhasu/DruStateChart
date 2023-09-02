// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartTypes.h"
#include "StructView.h"
#include "UObject/ObjectPtr.h"
#include "StateChartCondition.generated.h"

/*
 * Condition that decides whether transition should be taken
 */
USTRUCT()
struct DRUSTATECHART_API FStateChartCondition
{
    GENERATED_BODY()

public:
    virtual ~FStateChartCondition() = default;

    /*
     * Return true if Transition should be taken, false otherwise
     * TransitionEvent contains event that triggered the transition. It may be null in case of automatic transition.
     * 
     * Context is optional object passed from FStateChartExecutor. You may use it to pass any game specific data
     */
    virtual bool Evaluate(const FStateChartExecutionContext& Context, FConstStructView TransitionEvent) const
    {
        return true;
    }
};