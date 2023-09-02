// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "StateChartTypes.h"
#include "Impl/StateChartNodes.h"
#include "StateChartAsset.generated.h"

class UBaseStateDefinition;
class UTransitionDefinition;

UCLASS()
class DRUSTATECHART_API UStateChartAsset : public UObject
{
    GENERATED_BODY()

public:
    /* Helper method to create StateChart asset from States and Transitions. Do not use it direcly, use FStateChartBuilder instead */
    static TObjectPtr<UStateChartAsset> Create(const TArray<TObjectPtr<UBaseStateDefinition>>& States, const TArray<TObjectPtr<UTransitionDefinition>>& Transitions);

    void Serialize(FStructuredArchive::FRecord Record) override;
    void PostLoad() override;

    /* Returns default type of Continuation used in this StateChart */
    EActionContinuationType GetDefaultContinuationType() const { return DefaultContinuationType; }

    /* Returns assembled tree of nodes used by StateChartExecutor */
    const StateChart_Impl::FStateChartNodes& GetAssembledNodes() const;

protected:
    UFUNCTION(CallInEditor)
    void AssembleNodeTree();

#if WITH_EDITOR
    void MoveSubObjectsToExternalPackage();
#endif

private:
    friend class FStateChartBuilder;
    friend class UStateChartAssetFactoryNew;
    friend class SStatesView;

    UPROPERTY(EditAnywhere)
    EActionContinuationType DefaultContinuationType = EActionContinuationType::FirstFinish;

    UPROPERTY(EditAnywhere, Transient, SkipSerialization)
    TArray<TObjectPtr<UBaseStateDefinition>> AllStates;

    UPROPERTY(EditAnywhere, Transient, SkipSerialization)
    TArray<TObjectPtr<UTransitionDefinition>> AllTransitions;

    StateChart_Impl::FStateChartNodes Nodes;
    bool bNodesDirty = true;
};