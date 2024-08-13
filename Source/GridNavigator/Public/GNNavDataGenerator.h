#pragma once
#include "GNRecastNavMesh.h"
#include "AI/NavDataGenerator.h"

class FGNDataBuildTask final : public FNonAbandonableTask
{
public:
	TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(GNDataBuildTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	FORCEINLINE bool CanAbandon() const
	{
		return false;
	}

	void DoWork();
};

class GRIDNAVIGATOR_API FGNNavDataGenerator final : public FNavDataGenerator
{
public:
	FGNNavDataGenerator();
	explicit FGNNavDataGenerator(AGNRecastNavMesh* NavData);

	/**
	 * @brief Triggers a full rebuild of all navigation data associated with this generator
	 * 
	 * @return \c true if full rebuild was triggered successfully; \c false otherwise
	 */
	virtual bool RebuildAll() override;

	/**
	 * @brief Blocks until current build is complete
	 */
	virtual void EnsureBuildCompletion() override;

	/**
	 * @brief Cancels any current build, potentially blocking until async tasks are finished
	 *
	 * @note Unimplemented for now, since build cancellation is not supported
	 */
	virtual void CancelBuild() override;

	/**
	 * @brief Rebuilds portions of map data corresponding to updated navigation bounds
	 */
	virtual void OnNavigationBoundsChanged() override;

	/**
	 * @brief Rebuilds areas that have been updated/marked 'dirty'
	 * 
	 * @param DirtyAreas Navigation areas that have been updated and marked 'dirty'
	 */
	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas) override;

	/**
	 * @brief Checks whether a build is in progress due to dirty/updated tiles
	 * 
	 * @return \c true if a build is in progress due to dirty tiles; \c false otherwise
	 */
	virtual bool IsBuildInProgressCheckDirty() const override;

	/**
	 * @return Number of tasks that are waiting to be built 
	 */
	virtual int32 GetNumRemaningBuildTasks() const override;

	/**
	 * @return Number of tasks that are currently being built 
	 */
	virtual int32 GetNumRunningBuildTasks() const override;

private:
	typedef FAsyncTask<FGNDataBuildTask> FGNAsyncBuildTask;
	
	AGNRecastNavMesh* LinkedNavData = nullptr;
	TUniquePtr<FGNAsyncBuildTask> CurrentBuildTask = nullptr;
};
