// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateHandler.h"
#include "InstancedStruct.h"
#include "TestStateHandler.generated.h"

UCLASS()
class UTestStateHandler : public UStateHandler
{
    GENERATED_BODY()

public:
    void StateEntered(FConstStructView Event, const FStateChartExecutionContext& Context) override
    {
        bEnteredCalled = true;
        EnterEvent = Event;
    }

    void StateExited(const FStateChartExecutionContext& Context) override
    {
        bExitedCalled = true;
    }

    bool bEnteredCalled = false;
    bool bExitedCalled = false;

    FInstancedStruct EnterEvent;
};