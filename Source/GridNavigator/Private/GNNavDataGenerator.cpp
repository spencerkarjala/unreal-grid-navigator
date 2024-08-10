#include "GNNavDataGenerator.h"

FGNNavDataGenerator::FGNNavDataGenerator() : LinkedNavData(nullptr) {}

FGNNavDataGenerator::FGNNavDataGenerator(AGNRecastNavMesh* NavData) : LinkedNavData(NavData) {}

bool FGNNavDataGenerator::RebuildAll()
{
	return false;
}

void FGNNavDataGenerator::EnsureBuildCompletion()
{
	FNavDataGenerator::EnsureBuildCompletion();
}

void FGNNavDataGenerator::CancelBuild()
{
	FNavDataGenerator::CancelBuild();
}

void FGNNavDataGenerator::TickAsyncBuild(float DeltaSeconds)
{
	FNavDataGenerator::TickAsyncBuild(DeltaSeconds);
}
