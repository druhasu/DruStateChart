// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "StateChartAction.h"
#include "Templates/Function.h"
#include "TestActions.generated.h"

USTRUCT()
struct FTestAsyncAction : public FStateChartAction
{
    GENERATED_BODY()

public:
    FTestAsyncAction() = default;
    FTestAsyncAction(const TSharedPtr<FSimpleDelegate>& InTriggerPtr)
    {
        TriggerPtr = InTriggerPtr;
    }

    EActionContinuationType ExecuteAsync(const FStateChartExecutionContext& Context, const FSimpleDelegate& InDone) override
    {
        TriggerPtr->BindRaw(this, &FTestAsyncAction::OnTrigger);
        Done = InDone;
        return EActionContinuationType::FirstFinish;
    }

    void OnTrigger()
    {
        Done.ExecuteIfBound();
        TriggerPtr->Unbind();
    }

    FSimpleDelegate Done;
    TSharedPtr<FSimpleDelegate> TriggerPtr;
};

USTRUCT()
struct FTestCallbackAction : public FStateChartAction
{
    GENERATED_BODY()

public:
    FTestCallbackAction() = default;
    FTestCallbackAction(TFunction<void()> Action) : Action([Action = MoveTemp(Action)](const FStateChartExecutionContext&) { Action(); }) {}
    FTestCallbackAction(TFunction<void(const FStateChartExecutionContext&)> Action) : Action(MoveTemp(Action)) {}

    void Execute(const FStateChartExecutionContext& Context) override
    {
        if (Action)
        {
            Action(Context);
        }
    }

    TFunction<void(const FStateChartExecutionContext& Context)> Action;
};