// Copyright Andrei Sudarikov. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "StateChartCondition.h"
#include "StateChartEvent.generated.h"

USTRUCT()
struct DRUSTATECHART_API FStateChartGenericEvent
{
	GENERATED_BODY()

public:
	FStateChartGenericEvent() = default;
	FStateChartGenericEvent(const FGameplayTag& InTag) : EventTag(InTag) {}

	const FGameplayTag& GetEventTag() const { return EventTag; }

protected:
	UPROPERTY(EditAnywhere)
	FGameplayTag EventTag;
};

USTRUCT()
struct DRUSTATECHART_API FStateChartGenericEventCondition : public FStateChartCondition
{
	GENERATED_BODY()

public:
	FStateChartGenericEventCondition() = default;
	FStateChartGenericEventCondition(FGameplayTag EventTag, bool bMatchExact = false)
		: EventTag(MoveTemp(EventTag)), bMatchExact(bMatchExact)
	{};

	bool Evaluate(const FStateChartExecutionContext& Context, FConstStructView Event) const override;

	UPROPERTY(EditAnywhere)
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere)
	bool bMatchExact = false;
};