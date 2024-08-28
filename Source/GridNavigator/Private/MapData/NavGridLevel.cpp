#include "MapData/NavGridLevel.h"

#include "MapData/NavGridDataSerializer.h"

#include <sstream>

#include "GridNavigatorConfig.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridLevel, Log, All);

FString FNavGridLevel::ToString() const
{
	std::stringstream SStream;

	for (const auto& [ID, Block] : Blocks) {
		SStream
			<< "BlkID: " << ID
			<< "from (" << Block.Bounds.Min.X << ", " << Block.Bounds.Min.Y << ", " << Block.Bounds.Min.Z
			<< ") to (" << Block.Bounds.Max.X << ", " << Block.Bounds.Max.Y << ", " << Block.Bounds.Max.Z
			<< ")\r\n";
	}

	FString Result(SStream.str().c_str());
	return Result;
}

TArray<FVector> FNavGridLevel::FindPath(const FVector& From, const FVector& To)
{
	UE_LOG(LogNavGridLevel, Log, TEXT("Level got FindPath from '%s' to '%s'"), *From.ToString(), *To.ToString());
	if (Blocks.Num() == 0) {
		return {};
	}

	FNavGridBlock* StartBlock = nullptr;
	for (auto& [ID, Block] : Blocks) {
		if (Block.Bounds.IsInsideOrOn(From)) {
			StartBlock = &Block;
		}
	}

	// if no start block was found, then the start point isn't in a navigable area
	if (StartBlock == nullptr) {
		return {};
	}

	const FIntVector3 FromIndex = GridNavigatorConfig::WorldToGridIndex(From);
	const FIntVector3 ToIndex   = GridNavigatorConfig::WorldToGridIndex(To);

	TArray<FVector> PathPoints = StartBlock->Data.FindPath(FromIndex, ToIndex);

	for (auto& Point : PathPoints) {
		Point = GridNavigatorConfig::GridIndexToWorld(Point);
	}

	return PathPoints;
}
