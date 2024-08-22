#include "MapData/NavGridLevel.h"

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
