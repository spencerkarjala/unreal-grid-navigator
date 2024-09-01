#include "NavigationGridDataGenerator.h"

#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "MapData/NavGridLevel.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavigationGridDataGenerator, Log, All);

FNavigationGridDataGenerator::FNavigationGridDataGenerator() {}

FNavigationGridDataGenerator::FNavigationGridDataGenerator(ANavigationGridData* NavData) : LinkedNavData(NavData) {}

FNavigationGridDataGenerator::~FNavigationGridDataGenerator()
{
	if (CurrentBuildTask) {
		CurrentBuildTask->EnsureCompletion();
		CurrentBuildTask.Reset();
	}
}

bool FNavigationGridDataGenerator::RebuildAll()
{
	if (CurrentBuildTask.IsValid()) {
		CurrentBuildTask->EnsureCompletion();
	}

	UE_LOG(LogNavigationGridDataGenerator, Log, TEXT("Running build for navigation data: %s"), *LinkedNavData->GetPathName());

	CurrentBuildTask = MakeUnique<FAsyncBuildTask>(FNavGridBuildTask(GetWorld(), LinkedNavData));
	check(CurrentBuildTask.IsValid());
	CurrentBuildTask->GetTask().OnCompleted.BindSP(this, &FNavigationGridDataGenerator::HandleBuildCompleted);
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

	// determine whether a nav volume has been deleted
	// first, get a list of all of the existing level blocks
	TMap<uint32, bool> BoundIsRegistered;
	for (const auto& [LevelBlockID, Value] : LinkedNavData->LevelData->Blocks) {
		BoundIsRegistered.Add(LevelBlockID, false);
	}

	// now, cross-reference against the registered bounds
	for (const auto& RegisteredBound : RegisteredBoundsForThisData) {
		BoundIsRegistered[RegisteredBound.UniqueID] = true;
	}

	// if any bounds weren't matched in previous step, they must have been deleted
	for (const auto& [LevelBlockID, IsBlockRegistered] : BoundIsRegistered) {
		if (!IsBlockRegistered) {
			UE_LOG(LogNavigationGridDataGenerator, Log, TEXT("Found unregistered navigation block '%u'; removing its level data"), LevelBlockID);
			LinkedNavData->LevelData->RemoveBlock(LevelBlockID);
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

void FNavigationGridDataGenerator::HandleBuildCompleted() const
{
	if (LinkedNavData != nullptr && LinkedNavData->RenderingComp) {
		LinkedNavData->RenderingComp->MarkRenderStateDirty();
	}
	UE_LOG(LogNavigationGridDataGenerator, Log, TEXT("Completed build for navigation data: %s"), *LinkedNavData->GetPathName());
}
