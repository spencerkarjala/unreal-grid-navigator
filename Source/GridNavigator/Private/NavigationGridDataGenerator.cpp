#include "NavigationGridDataGenerator.h"

#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "MapData/NavGridLevel.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavigationGridDataGenerator, Log, All);

FNavigationGridDataGenerator::FNavigationGridDataGenerator() {}

FNavigationGridDataGenerator::FNavigationGridDataGenerator(ANavigationGridData* NavData) : LinkedNavData(NavData) {}

bool FNavigationGridDataGenerator::RebuildAll()
{
	if (CurrentBuildTask.IsValid()) {
		CurrentBuildTask->EnsureCompletion();
	}

	UE_LOG(LogNavigationGridDataGenerator, Log, TEXT("Running build for navigation data: %s"), *LinkedNavData->GetPathName());

	CurrentBuildTask = MakeUnique<FGNAsyncBuildTask>(FNavGridBuildTask(GetWorld(), LinkedNavData));
	check(CurrentBuildTask.IsValid());
	CurrentBuildTask->StartBackgroundTask();

	return true;
}

void FNavigationGridDataGenerator::EnsureBuildCompletion()
{
	if (!CurrentBuildTask) {
		return;
	}

	CurrentBuildTask->EnsureCompletion();
}

void FNavigationGridDataGenerator::CancelBuild() {}

void FNavigationGridDataGenerator::OnNavigationBoundsChanged()
{
	// RebuildAll();
}

void FNavigationGridDataGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
	if (!LinkedNavData) {
		UE_LOG(LogNavigationGridDataGenerator, Error, TEXT("Tried to RebuildDirtyAreas without a linked ANavigationData instance"));
		return;
	}
	
	const auto* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!IsValid(NavSys)) {
		UE_LOG(LogNavigationGridDataGenerator, Error, TEXT("Failed to retrieve navigation system during RebuildDirtyAreas"));
		return;
	}
	const auto& RegisteredBounds = NavSys->GetNavigationBounds();

	// pre-filter registered bounds to get rid of any that may not be associated with this NavigationData instance
	// would be nice to use UNavigationSystemV1::GetNavigationBoundsForNavData, but it strips ID information before return
	TArray<FNavigationBounds> RegisteredBoundsForThisData;
	RegisteredBoundsForThisData.Reserve(RegisteredBounds.Num());
	const int32 AgentIndex = NavSys->GetSupportedAgentIndex(LinkedNavData);
	const auto* LinkedLevel = LinkedNavData->GetLevel();
	for (const auto& RegisteredBound : RegisteredBounds) {
		if ((LinkedLevel == nullptr || RegisteredBound.Level == LinkedLevel) && RegisteredBound.SupportedAgents.Contains(AgentIndex)) {
			RegisteredBoundsForThisData.Add(RegisteredBound);
		}
	}

	if (RegisteredBoundsForThisData.Num() == 0) {
		return;
	} 

	for (const auto& [UniqueID, AreaBox, SupportedAgents, Level] : RegisteredBoundsForThisData) {
		const auto* BlockData = LinkedNavData->LevelData->GetBlock(UniqueID);

		if (BlockData == nullptr) {
			LinkedNavData->LevelData->AddBlock(UniqueID, FNavGridBlock(AreaBox));
			continue;
		}

		if (!BlockData->Bounds.Equals(AreaBox, 0.001)) {
			LinkedNavData->LevelData->UpdateBlock(UniqueID, FNavGridBlock(AreaBox));
		}
	}

	RebuildAll();
}

bool FNavigationGridDataGenerator::IsBuildInProgressCheckDirty() const
{
	return FNavDataGenerator::IsBuildInProgressCheckDirty();
}

int32 FNavigationGridDataGenerator::GetNumRemaningBuildTasks() const
{
	return CurrentBuildTask.IsValid() && CurrentBuildTask->IsIdle() ? 1 : 0;
}

int32 FNavigationGridDataGenerator::GetNumRunningBuildTasks() const
{
	return CurrentBuildTask.IsValid() && !CurrentBuildTask->IsWorkDone() ? 1 : 0;
}
