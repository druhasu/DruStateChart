// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Impl/StateChartNodes.h"
#include "Impl/StateChartElements.h"
#include "Impl/StateChartDefaultExecutor.h"
#include "StateChartAsset.h"
#include "StateChartBuilder.h"
#include "StateChartEvent.h"

#include "TestActions.h"
#include "TestEvents.h"
#include "TestStateHandler.h"

BEGIN_DEFINE_SPEC(FStateChartExecutorSpec, "DruStateChart.StateChart Executor", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter)

bool TestActive(const FString& State, const DruStateChart_Impl::FStateChartDefaultExecutor& Executor);
bool TestNotActive(const FString& State, const DruStateChart_Impl::FStateChartDefaultExecutor& Executor);

END_DEFINE_SPEC(FStateChartExecutorSpec)

void FStateChartExecutorSpec::Define()
{
    using namespace DruStateChart_Impl;

    Describe("Initial", [this]
    {
        It("Should Activate Default Initial State", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a"), // <-- this will be initial state
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();

            TestActive("root", *Executor);
            TestActive("a", *Executor);
        });

        It("Should Activate Specific Initial State", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Initial("b").Children
            (
                Builder.State("a"),
                Builder.State("b") // <-- this will be initial state
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();

            TestActive("root", *Executor);
            TestActive("b", *Executor);
        });

        It("Should Activate Specific Initial State via Transition", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("s").Children
                (
                    Builder.State("a"),
                    Builder.State("b"), // <-- this will be initial state

                    Builder.Transition().Target("s.b").Initial()
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();

            TestActive("root", *Executor);
            TestActive("s", *Executor);
            TestActive("b", *Executor);
        });

        It("Should Activate Initial Parallel State", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.Parallel("p").Children
                (
                    Builder.State("a"), // <-- this will be initial state
                    Builder.State("b")  // <-- this will be initial state
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();

            TestActive("root", *Executor);
            TestActive("p", *Executor);
            TestActive("a", *Executor);
            TestActive("b", *Executor);
        });
    });

    Describe("Transitions", [this]
    {
        It("Should Transition To Other State", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>()
                ),
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("root", *Executor);
            TestActive("b", *Executor);
        });

        It("Should Transition To Other State Async", [this]
        {
            TSharedPtr<FSimpleDelegate> Trigger = MakeShared<FSimpleDelegate>();
            FTestAsyncAction Action(Trigger);

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>().Action(Action)
                ),
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("root", *Executor);
            TestNotActive("b", *Executor);

            Trigger->ExecuteIfBound();

            TestActive("root", *Executor);
            TestActive("b", *Executor);
        });

        It("Should Select Topmost Transition", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children
                (
                    Builder.State("1").Children // <-- this will be initial state
                    (
                        Builder.Transition().Target("c").Event<FTestEvent>()
                    ),

                    Builder.Transition().Target("b").Event<FTestEvent>()
                ),
                Builder.State("b"),
                Builder.State("c")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("root", *Executor);
            TestNotActive("b", *Executor);
            TestActive("c", *Executor);
        });

        It("Should Select First Transition if Ambiguous", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.Parallel("p").Children
                (
                    Builder.State("a").Children // <-- this will be initial state
                    (
                        Builder.Transition().Target("c").Event<FTestEvent>(),
                        Builder.Transition().Target("d").Event<FTestEvent>()
                    ),
                    Builder.State("b").Children // <-- this will be initial state
                    (
                        Builder.Transition().Target("e").Event<FTestEvent>()
                    ),

                    Builder.Transition().Target("f").Event<FTestEvent>()
                ),

                Builder.State("c"),
                Builder.State("d"),
                Builder.State("e"),
                Builder.State("f")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("root", *Executor);
            TestActive("c", *Executor);
            TestNotActive("d", *Executor);
            TestNotActive("e", *Executor);
            TestNotActive("f", *Executor);
        });

        It("Should Transition To Other Parallel State", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.Parallel("p1").Children
                (
                    Builder.State("a"), // <-- this will be initial state
                    Builder.State("b"), // <-- this will be initial state

                    Builder.Transition().Target("p2").Event<FTestEvent>()
                ),
                Builder.Parallel("p2").Children
                (
                    Builder.State("c"),
                    Builder.State("d")
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("root", *Executor);
            TestActive("p2", *Executor);
            TestActive("c", *Executor);
            TestActive("d", *Executor);

            TestNotActive("p1", *Executor);
            TestNotActive("a", *Executor);
            TestNotActive("b", *Executor);
        });
    });

    Describe("StateHandler", [this]
    {
        It("Should Call StateEntered on StateHandler", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Handler<UTestStateHandler>(), // <-- this will be initial state
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            UTestStateHandler* InstancedHandler = nullptr;
            Executor->OnStateHandlerCreated().AddLambda([&](UStateHandler& InHandler)
            {
                InstancedHandler = Cast<UTestStateHandler>(&InHandler);
            });

            Executor->Execute();

            TestNotNull("InstancedHandler", InstancedHandler);
            TestTrue("Called StateEntered", InstancedHandler->bEnteredCalled);
            TestFalse("Called StateExited", InstancedHandler->bExitedCalled);
        });

        It("Should Call StateExited on StateHandler", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Handler<UTestStateHandler>().Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>()
                ),
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            UTestStateHandler* InstancedHandler = nullptr;
            Executor->OnStateHandlerCreated().AddLambda([&](UStateHandler& InHandler)
            {
                InstancedHandler = Cast<UTestStateHandler>(&InHandler);
            });

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestNotNull("InstancedHandler", InstancedHandler);
            TestTrue("Called StateEntered", InstancedHandler->bEnteredCalled);
            TestTrue("Called StateExited", InstancedHandler->bExitedCalled);
        });

        It("Should Pass Event to StateEntered", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Event<FTestEvent>().Target("b")
                ),
                Builder.State("b").Handler<UTestStateHandler>()
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            UTestStateHandler* InstancedHandler = nullptr;
            Executor->OnStateHandlerCreated().AddLambda([&](UStateHandler& InHandler)
            {
                InstancedHandler = Cast<UTestStateHandler>(&InHandler);
            });

            Executor->Execute();
            Executor->ExecuteEvent(FTestEvent("TestEvent"));

            TestNotNull("InstancedHandler", InstancedHandler);
            TestTrue("Event is Valid", InstancedHandler->EnterEvent.IsValid());
            TestEqual("Event Name", InstancedHandler->EnterEvent.Get<FTestEvent>().Name, FName("TestEvent"));
        });
    });

    Describe("Actions", [this]
    {
        It("Should Call OnEnter Actions", [this]
        {
            bool bCalled = false;
            FTestCallbackAction Action([&] { bCalled = true; });

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>()
                ),
                Builder.State("b").OnEnter(Action)
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestTrue("Action called", bCalled);
        });

        It("Should Call OnExit Actions", [this]
        {
            bool bCalled = false;
            FTestCallbackAction Action([&] { bCalled = true; });

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").OnExit(Action).Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>()
                ),
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestTrue("Action called", bCalled);
        });

        It("Should Call Transition Actions", [this]
        {
            bool bCalled = false;
            FTestCallbackAction Action([&] { bCalled = true; });

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Action(Action).Target("b").Event<FTestEvent>()
                ),
                Builder.State("b")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestTrue("Action called", bCalled);
        });

        It("Should Call Initial Transition Actions", [this]
        {
            bool bCalled = false;
            FTestCallbackAction Action([&] { bCalled = true; });

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("s").Children
                (
                    Builder.State("a"),
                    Builder.State("b"), // <-- this will be initial state

                    Builder.Transition().Action(Action).Target("s.b").Initial()
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();

            TestTrue("Action called", bCalled);
        });
    });

    Describe("History", [this]
    {
        It("Should Activate History Initial State", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("initial").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("s.h").Event<FTestEvent>()
                ),
                Builder.State("s").Children
                (
                    Builder.History("h"),
                    Builder.State("h_initial")
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("h_initial", *Executor);
        });

        It("Should Restore Shallow History", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("initial").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("s.a.c").Event<FTestEvent>() // <-- transition 1
                ),
                Builder.State("s").Children
                (
                    Builder.History("h").HistoryType(EHistoryType::Shallow).Initial("h_initial"),
                    Builder.State("h_initial"),

                    Builder.State("a").Children
                    (
                        Builder.State("b"),
                        Builder.State("c")
                    ),

                    Builder.Transition().Target("temp").Event<FTestEvent>() // <-- transition 2
                ),
                Builder.State("temp").Children
                (
                    Builder.Transition().Target("s.h").Event<FTestEvent>() // <-- transition 3
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>(); // <-- transition 1
            Executor->ExecuteEvent<FTestEvent>(); // <-- transition 2
            Executor->ExecuteEvent<FTestEvent>(); // <-- transition 3

            TestActive("a", *Executor); // should be restored by History state "h"
            TestActive("b", *Executor); // should be restored by History state "h"
            TestNotActive("c", *Executor); // should not be restored by History state "h", due to it being 'shallow'
        });

        It("Should Restore Deep History", [this]
        {
            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("initial").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("s.a.c").Event<FTestEvent>() // <-- transition 1
                ),
                Builder.State("s").Children
                (
                    Builder.History("h").HistoryType(EHistoryType::Deep).Initial("h_initial"),
                    Builder.State("h_initial"),

                    Builder.State("a").Children
                    (
                        Builder.State("b"),
                        Builder.State("c")
                    ),

                    Builder.Transition().Target("temp").Event<FTestEvent>() // <-- transition 2
                ),
                Builder.State("temp").Children
                (
                    Builder.Transition().Target("s.h").Event<FTestEvent>() // <-- transition 3
                )
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>(); // <-- transition 1
            Executor->ExecuteEvent<FTestEvent>(); // <-- transition 2
            Executor->ExecuteEvent<FTestEvent>(); // <-- transition 3

            TestActive("c", *Executor); // should be restored by History state "h", due to it being 'deep'
        });
    });

    Describe("Event Queue", [this]
    {
        It("Should Execute Queued External Event", [this]
        {
            TSharedPtr<FSimpleDelegate> Trigger = MakeShared<FSimpleDelegate>();
            FTestAsyncAction Action(Trigger);

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>().Action(Action)
                ),
                Builder.State("b").Children
                (
                    Builder.Transition().Target("c").Event<FTestEvent>()
                ),
                Builder.State("c")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();
            Executor->ExecuteEvent<FTestEvent>(); // <-- this event will be put in external event queue

            TestActive("root", *Executor);
            TestNotActive("c", *Executor);

            Trigger->ExecuteIfBound();

            TestActive("root", *Executor);
            TestActive("c", *Executor);
        });

        It("Should Execute Queued Internal Event", [this]
        {
            TSharedPtr<FSimpleDelegate> Trigger = MakeShared<FSimpleDelegate>();
            FTestAsyncAction Action(Trigger);

            FTestCallbackAction EventAction([&](const FStateChartExecutionContext& Context)
            {
                Context.Executor.ExecuteEvent<FTestEvent>(); // <-- this event will be put in internal event queue
            });

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>().Action(Action).Action(EventAction)
                ),
                Builder.State("b").Children
                (
                    Builder.Transition().Target("c").Event<FTestEvent>()
                ),
                Builder.State("c")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();

            TestActive("root", *Executor);
            TestNotActive("c", *Executor);

            Trigger->ExecuteIfBound();

            TestActive("root", *Executor);
            TestActive("c", *Executor);
        });

        It("Should Execute Queued Internal then External Event", [this]
        {
            TSharedPtr<FSimpleDelegate> Trigger = MakeShared<FSimpleDelegate>();
            FTestAsyncAction Action(Trigger);

            FTestCallbackAction EventAction([&](const FStateChartExecutionContext& Context)
            {
                Context.Executor.ExecuteEvent(FTestEvent("Event1")); // <-- this event will be put in internal event queue
            });

            FStateChartBuilder Builder;
            Builder.Root().Children
            (
                Builder.State("a").Children // <-- this will be initial state
                (
                    Builder.Transition().Target("b").Event<FTestEvent>().Action(Action).Action(EventAction)
                ),
                Builder.State("b").Children
                (
                    Builder.Transition().Target("c").Event<FTestEvent>().Condition(FTestCondition("Event1")),
                    Builder.Transition().Target("err").Event<FTestEvent>().Condition(FTestCondition("Event2")) // if events trigger in incorrect order, "err" state will be entered
                ),
                Builder.State("c").Children
                (
                    Builder.Transition().Target("d").Event<FTestEvent>().Condition(FTestCondition("Event2"))
                ),
                Builder.State("d"),
                Builder.State("err")
            );

            TObjectPtr<UStateChartAsset> StateChart = Builder.Build();
            TSharedRef<FStateChartDefaultExecutor> Executor = MakeShared<FStateChartDefaultExecutor>(*StateChart);

            Executor->Execute();
            Executor->ExecuteEvent<FTestEvent>();
            Executor->ExecuteEvent(FTestEvent("Event2")); // <-- this event will be put in external event queue

            TestActive("root", *Executor);
            TestNotActive("c", *Executor);
            TestNotActive("d", *Executor);
            TestNotActive("err", *Executor);

            Trigger->ExecuteIfBound();

            TestActive("root", *Executor);
            TestActive("d", *Executor);
        });
    });
}

bool FStateChartExecutorSpec::TestActive(const FString& State, const DruStateChart_Impl::FStateChartDefaultExecutor& Executor)
{
    return TestTrue(FString::Printf(TEXT("'%s' Active"), *State), Executor.GetActiveStates().ContainsByPredicate([&](auto S) { return S->FriendlyName == State; }));
}

bool FStateChartExecutorSpec::TestNotActive(const FString& State, const DruStateChart_Impl::FStateChartDefaultExecutor& Executor)
{
    return TestFalse(FString::Printf(TEXT("'%s' Active"), *State), Executor.GetActiveStates().ContainsByPredicate([&](auto S) { return S->FriendlyName == State; }));
}