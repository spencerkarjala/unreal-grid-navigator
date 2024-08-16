#include "NavGridBlock.h"

namespace NavGrid
{
	FBlock::FBlock() {}
	FBlock::FBlock(const FBox& NewBounds) : Bounds(NewBounds) {}
}