#pragma once

namespace NavGrid
{
	class FBlock
	{
	public:
		FBlock();
		explicit FBlock(const FBox& NewBounds);

		FBox Bounds;
	};
}