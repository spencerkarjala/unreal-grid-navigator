#pragma once

#include "CoreMinimal.h"
#include "GridNavigator/Private/MapData/NavGridAdjacencyList.h"
#include "NavGridBlock.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FNavGridBlock
{
	GENERATED_BODY()
	
	FNavGridBlock();
	explicit FNavGridBlock(const FBox& NewBounds);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBox Bounds;

	FNavGridAdjacencyList Data;
};
