// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartCondition.h"
#include "TestEvents.generated.h"

USTRUCT()
struct FTestEvent
{
    GENERATED_BODY()

public:
    FTestEvent() = default;
    FTestEvent(FName Name) : Name(Name) {}

    FName Name;
};

USTRUCT()
struct FTestCondition : public FStateChartCondition
{
    GENERATED_BODY()

public:
    FTestCondition() = default;
    FTestCondition(FName Name) : Name(Name) {}

    bool Evaluate(const FStateChartExecutionContext& Context, FConstStructView TransitionEvent) const override
    {
        const FTestEvent* Event = TransitionEvent.GetPtr<FTestEvent>();

        return !Event || Event->Name == Name;
    }

    FName Name;
};