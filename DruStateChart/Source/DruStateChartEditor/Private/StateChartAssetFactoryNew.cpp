// Copyright Andrei Sudarikov. All Rights Reserved.

#include "StateChartAssetFactoryNew.h"
#include "StateChartAsset.h"
#include "StateChartBuilder.h"
#include "Impl/StateChartElements.h"

UStateChartAssetFactoryNew::UStateChartAssetFactoryNew()
{
    SupportedClass = UStateChartAsset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UStateChartAssetFactoryNew::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    check(InClass->IsChildOf(UStateChartAsset::StaticClass()));

    FStateChartBuilder Builder;
    Builder.Root().Children
    (
        Builder.Parallel("p1").Children
        (
            Builder.State("a"), // <-- this will be initial state
            Builder.State("b") // <-- this will be initial state
        ),
        Builder.Parallel("p2").Children
        (
            Builder.State("c"),
            Builder.State("d")
        )
    );

    TObjectPtr<UStateChartAsset> Result = Builder.Build(InParent, InClass, InName, Flags);
    Result->MoveSubObjectsToExternalPackage();

    return Result;
}

bool UStateChartAssetFactoryNew::ShouldShowInNewMenu() const
{
    return true;
}