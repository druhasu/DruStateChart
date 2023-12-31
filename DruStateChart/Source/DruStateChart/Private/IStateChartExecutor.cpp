// Copyright Andrei Sudarikov. All Rights Reserved.

#include "Interfaces/IStateChartExecutor.h"
#include "Impl/StateChartDefaultExecutor.h"

TSharedRef<IStateChartExecutor> IStateChartExecutor::CreateDefault(UStateChartAsset& StateChartAsset, TObjectPtr<UObject> ContextObject)
{
    return MakeShared<DruStateChart_Impl::FStateChartDefaultExecutor>(StateChartAsset, ContextObject);
}