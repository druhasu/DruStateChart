// Copyright Andrei Sudarikov. All Rights Reserved.

#include "StateChartEvent.h"

bool FStateChartGenericEventCondition::Evaluate(const FStateChartExecutionContext& Context, FConstStructView Event) const
{
	const FStateChartGenericEvent* GenericEvent = Event.GetPtr<FStateChartGenericEvent>();

	if (GenericEvent != nullptr)
	{
		return bMatchExact ?
			GenericEvent->GetEventTag().MatchesTagExact(EventTag) :
			GenericEvent->GetEventTag().MatchesTag(EventTag);
	}

	return false;
}