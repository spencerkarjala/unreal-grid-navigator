#include "GNNavDataGenerator.h"

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
	FNavDataGenerator::RebuildDirtyAreas(DirtyAreas);
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
