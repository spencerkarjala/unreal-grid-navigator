#pragma once

#include "CoreMinimal.h"
#include "NavGridBlock.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FNavGridBlock
{
	GENERATED_BODY()
	
	FNavGridBlock();
	explicit FNavGridBlock(const FBox& NewBounds);

	FBox Bounds;
};