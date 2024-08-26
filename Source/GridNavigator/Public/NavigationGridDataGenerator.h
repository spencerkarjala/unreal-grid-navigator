#pragma once
#include "NavigationGridData.h"
#include "AI/NavDataGenerator.h"

class FGNDataBuildTask final : public FNonAbandonableTask
{
public:
	FGNDataBuildTask(UWorld* World);
	
	TStatId GetStatId() const;
	FORCEINLINE bool CanAbandon() const;

	void DoWork() const;

private:
	TObjectPtr<UWorld> WorldRef;
};

class GRIDNAVIGATOR_API FNavigationGridDataGenerator final : public FNavDataGenerator
{
public:
	FNavigationGridDataGenerator();
	explicit FNavigationGridDataGenerator(ANavigationGridData* NavData);

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

protected:
	FORCEINLINE UWorld* GetWorld() const { return IsValid(LinkedNavData) ? LinkedNavData->GetWorld() : nullptr; }

private:
	typedef FAsyncTask<FGNDataBuildTask> FGNAsyncBuildTask;
	
	ANavigationGridData* LinkedNavData = nullptr;
	TUniquePtr<FGNAsyncBuildTask> CurrentBuildTask = nullptr;
};
