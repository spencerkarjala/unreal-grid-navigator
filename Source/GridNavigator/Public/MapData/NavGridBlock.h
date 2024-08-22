#pragma once

#include "CoreMinimal.h"
#include "NavGridBlock.generated.h"

USTRUCT()
struct FNavGridBlock
{
	GENERATED_BODY()
	
	FNavGridBlock();
	explicit FNavGridBlock(const FBox& NewBounds);

	FBox Bounds;
};