#pragma once
#include "NavigationGridData.h"

class FNavGridBuildTask : public FNonAbandonableTask
{
public:
	FNavGridBuildTask(UWorld* World, ANavigationGridData* Data);
	
	TStatId GetStatId() const;
	FORCEINLINE bool CanAbandon() const;

	void DoWork() const;

private:
	TObjectPtr<UWorld> WorldRef;
	TObjectPtr<ANavigationGridData> DataRef;
};
