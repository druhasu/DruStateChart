// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "StateChartAssetFactoryNew.generated.h"

UCLASS()
class UStateChartAssetFactoryNew : public UFactory
{
    GENERATED_BODY()

public:
    UStateChartAssetFactoryNew();

    UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    bool ShouldShowInNewMenu() const override;
};