#pragma once
#include "GNRecastNavMesh.h"
#include "AI/NavDataGenerator.h"

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
	 */
	virtual void CancelBuild() override;

	/**
	 * @brief Ticks any in-progress asynchronous builds
	 *
	 * @note If generator is set to time-sliced rebuild, then the function
	 */
	virtual void TickAsyncBuild(float DeltaSeconds) override;

private:
	AGNRecastNavMesh* LinkedNavData;
};
