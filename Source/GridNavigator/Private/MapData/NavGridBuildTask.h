#pragma once
#include "NavigationGridData.h"

DECLARE_DELEGATE(FNavGridBuildTaskDelegate)

class FNavGridBuildTask : public FNonAbandonableTask
{
public:
	FNavGridBuildTask(UWorld* World, ANavigationGridData* Data);
	
	TStatId GetStatId() const;
	FORCEINLINE bool CanAbandon() const;

	void DoWork() const;
	static void PopulateBlock(const UWorld& World, FNavGridAdjacencyList& Map, const FBox& BoundingBox);

	FNavGridBuildTaskDelegate OnCompleted;

private:
	TObjectPtr<UWorld> WorldRef;
	TObjectPtr<ANavigationGridData> DataRef;
};
