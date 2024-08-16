#include "GNNavDataGenerator.h"

#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "MapData/NavGridLevel.h"

DECLARE_LOG_CATEGORY_CLASS(LogGNNavDataGenerator, Log, All);

void FGNDataBuildTask::DoWork()
{
	UE_LOG(LogGNNavDataGenerator, Warning, TEXT("Starting work on FGNDataBuildTask; game time: %0.2f"), FPlatformTime::Seconds());
	FPlatformProcess::Sleep(2.f);
	UE_LOG(LogGNNavDataGenerator, Warning, TEXT("Work completed on FGNDataBuildTask; game time: %0.2f"), FPlatformTime::Seconds());
}

FGNNavDataGenerator::FGNNavDataGenerator() : LinkedNavData(nullptr) {}

FGNNavDataGenerator::FGNNavDataGenerator(AGNRecastNavMesh* NavData) : LinkedNavData(NavData) {}

bool FGNNavDataGenerator::RebuildAll()
{
	if (CurrentBuildTask.IsValid()) {
		CurrentBuildTask->EnsureCompletion();
	}

	CurrentBuildTask = MakeUnique<FGNAsyncBuildTask>(FGNDataBuildTask());
	check(CurrentBuildTask.IsValid());
	CurrentBuildTask->StartBackgroundTask();

	return true;
}

void FGNNavDataGenerator::EnsureBuildCompletion()
{
	if (!CurrentBuildTask) {
		return;
	}

	CurrentBuildTask->EnsureCompletion();
}

void FGNNavDataGenerator::CancelBuild() {}

void FGNNavDataGenerator::OnNavigationBoundsChanged()
{
	RebuildAll();
}

void FGNNavDataGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
	if (!LinkedNavData) {
		UE_LOG(LogGNNavDataGenerator, Error, TEXT("Tried to RebuildDirtyAreas without a linked ANavigationData instance"));
		return;
	}
	
	const auto* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!IsValid(NavSys)) {
		UE_LOG(LogGNNavDataGenerator, Error, TEXT("Failed to retrieve navigation system during RebuildDirtyAreas"));
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
			LinkedNavData->LevelData->AddBlock(UniqueID, NavGrid::FBlock(AreaBox));
			continue;
		}

		if (!BlockData->Bounds.Equals(AreaBox, 0.001)) {
			LinkedNavData->LevelData->UpdateBlock(UniqueID, NavGrid::FBlock(AreaBox));
		}
	}
	
	FString AreasString("\r\n");
	for (const auto& DirtyArea : DirtyAreas) {
		AreasString += DirtyArea.Bounds.ToString() + "\r\n";
	}
	AreasString += "aaa \r\n";
	for (const auto& Area : NavSys->GetNavigationBounds()) {
		AreasString += Area.AreaBox.ToString() + "\r\n";
	}
	UE_LOG(LogGNNavDataGenerator, Log, TEXT("Got RebuildDirtyAreas for: '%s'"), *AreasString);
	
	RebuildAll();
}

bool FGNNavDataGenerator::IsBuildInProgressCheckDirty() const
{
	return FNavDataGenerator::IsBuildInProgressCheckDirty();
}

int32 FGNNavDataGenerator::GetNumRemaningBuildTasks() const
{
	return CurrentBuildTask.IsValid() && CurrentBuildTask->IsIdle() ? 1 : 0;
}

int32 FGNNavDataGenerator::GetNumRunningBuildTasks() const
{
	return CurrentBuildTask.IsValid() && !CurrentBuildTask->IsWorkDone() ? 1 : 0;
}
