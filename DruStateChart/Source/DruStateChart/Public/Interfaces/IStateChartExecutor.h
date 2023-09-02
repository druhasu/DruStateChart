// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "StructView.h"

class UStateHandler;
class UStateChartAsset;
class UBaseStateDefinition;

/*
 * Executes StateChart. Intended to be stored inside TSharedRef
 */
class DRUSTATECHART_API IStateChartExecutor : public TSharedFromThis<IStateChartExecutor>
{
public:
    using FHandlerCreated = TMulticastDelegate<void(UStateHandler& NewHandler)>;

    virtual ~IStateChartExecutor() = default;

    /* Creates new Executor from given Asset and optional Context object*/
    static TSharedRef<IStateChartExecutor> CreateDefault(UStateChartAsset& StateChartAsset, TObjectPtr<UObject> ContextObject = nullptr);

    /* Returns StateChart that are being executed */
    virtual TObjectPtr<UStateChartAsset> GetExecutingAsset() const = 0;

    /* Starts asset execution */
    virtual void Execute() = 0;

    /* Executes Event with provided payload */
    template <typename T, TEMPLATE_REQUIRES(TModels<CStaticStructProvider, T>::Value)>
    void ExecuteEvent(const T& Event)
    {
        ExecuteEventImpl(FConstStructView::Make(Event));
    }

    /* Executes Event of requested type with default payload */
    template <typename T, TEMPLATE_REQUIRES(TModels<CStaticStructProvider, T>::Value)>
    void ExecuteEvent()
    {
        ExecuteEventImpl(FConstStructView(T::StaticStruct(), nullptr));
    }

    /* Executes Event of requested type with default payload */
    void ExecuteEvent(const UScriptStruct& EventType)
    {
        ExecuteEventImpl(FConstStructView(&EventType, nullptr));
    }

    /* Returns definitions of all active states */
    virtual TArray<TObjectPtr<UBaseStateDefinition>> GetActiveStates() const = 0;

    /* Called when new instance of StateHandler is created */
    virtual FHandlerCreated& OnStateHandlerCreated() = 0;

protected:
    virtual void ExecuteEventImpl(FConstStructView Event) = 0;
};