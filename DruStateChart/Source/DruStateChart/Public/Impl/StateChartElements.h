// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "InstancedStruct.h"
#include "StateChartTypes.h"
#include "StateChartElements.generated.h"

UCLASS()
class DRUSTATECHART_API UStateChartElementAsset : public UObject
{
    GENERATED_BODY()

public:
    bool IsAsset() const override
    {
        return !GetPackage()->HasAnyFlags(RF_Transient) && !HasAnyFlags(RF_Transient | RF_ClassDefaultObject);
    }
};

UCLASS(EditInlineNew, DefaultToInstanced)
class DRUSTATECHART_API UTransitionDefinition : public UStateChartElementAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FGuid SourceState;

    UPROPERTY(EditAnywhere)
    double SortOrder = 0;

    UPROPERTY(EditAnywhere)
    bool bInitial = false;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UScriptStruct> EventID;

    UPROPERTY(EditAnywhere)
    TArray<FGuid> TargetStates;

    UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/DruStateChart.StateChartCondition"))
    TArray<FInstancedStruct> Conditions;

    UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/DruStateChart.StateChartAction"))
    TArray<FInstancedStruct> Actions;
};


UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class DRUSTATECHART_API UBaseStateDefinition : public UStateChartElementAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "State")
    FGuid ID = FGuid::NewGuid();

    UPROPERTY(EditAnywhere, Category = "State")
    FGuid ParentID;

    UPROPERTY(EditAnywhere, Category = "State")
    FString FriendlyName;
};

UCLASS()
class DRUSTATECHART_API UHistoryStateDefinition : public UBaseStateDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "State")
    EHistoryType HistoryType = EHistoryType::Shallow;
};

UCLASS(Abstract)
class DRUSTATECHART_API UBaseStateWithActionsDefinition : public UBaseStateDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "State", meta = (BaseStruct = "/Script/DruStateChart.StateChartAction"))
    TArray<FInstancedStruct> EnterActions;

    UPROPERTY(EditAnywhere, Category = "State", meta = (BaseStruct = "/Script/DruStateChart.StateChartAction"))
    TArray<FInstancedStruct> ExitActions;
};

UCLASS()
class DRUSTATECHART_API UFinalStateDefinition : public UBaseStateWithActionsDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "State")
    FInstancedStruct DoneData;
};

UCLASS(Abstract)
class DRUSTATECHART_API UActivatableStateDefinition : public UBaseStateWithActionsDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "State")
    TArray<TObjectPtr<class UStateHandler>> Handlers;

    UPROPERTY(EditAnywhere, Category = "State")
    double SortOrder = 0;
};

UCLASS()
class DRUSTATECHART_API UCompoundStateDefinition : public UActivatableStateDefinition
{
    GENERATED_BODY()
};

UCLASS()
class DRUSTATECHART_API UParallelStateDefinition : public UActivatableStateDefinition
{
    GENERATED_BODY()
};