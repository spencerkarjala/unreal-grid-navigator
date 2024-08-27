#pragma once
#include "NavigationGridData.h"

class FNavGridBuildTask : public FNonAbandonableTask
{
public:
	FNavGridBuildTask(UWorld* World, ANavigationGridData* Data);
	
	TStatId GetStatId() const;
	FORCEINLINE bool CanAbandon() const;

	void DoWork() const;
	static void PopulateBlock(const UWorld& World, FNavGridBlock& Block);

private:
	TObjectPtr<UWorld> WorldRef;
	TObjectPtr<ANavigationGridData> DataRef;
};
