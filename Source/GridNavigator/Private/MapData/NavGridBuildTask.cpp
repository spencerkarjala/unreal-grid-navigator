#include "NavGridBuildTask.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridBuildTask, Log, All);

FNavGridBuildTask::FNavGridBuildTask(UWorld* World, ANavigationGridData* Data) : WorldRef(World), DataRef(Data) {}

TStatId FNavGridBuildTask::GetStatId() const 
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(GNDataBuildTask, STATGROUP_ThreadPoolAsyncTasks);
}

bool FNavGridBuildTask::CanAbandon() const 
{
	return false;
}

void FNavGridBuildTask::DoWork() const
{
	if (!WorldRef) {
		UE_LOG(LogNavGridBuildTask, Error, TEXT("Tried to rebuild navigation grid data without a valid world reference"));
		return;
	}
	if (!DataRef) {
		UE_LOG(LogNavGridBuildTask, Error, TEXT("Tried to rebuild navigation grid data without a valid data reference"));
		return;
	}

	auto DataBlocks = DataRef->GetNavigationBlocks();
	for (const auto& Block : DataBlocks) {
		UE_LOG(LogNavGridBuildTask, Warning, TEXT("data: %s"), *Block.Bounds.ToString());
		
	}
	// UE_LOG(LogNavGridBuildTask, Warning, TEXT("Starting work on NavGridBuildTask; game time: %0.2f"), FPlatformTime::Seconds());
	// FPlatformProcess::Sleep(2.f);
	// UE_LOG(LogNavGridBuildTask, Warning, TEXT("Work completed on NavGridBuildTask; game time: %0.2f"), FPlatformTime::Seconds());
}
