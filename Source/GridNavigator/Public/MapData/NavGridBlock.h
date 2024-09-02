#pragma once

#include "CoreMinimal.h"
#include "GridNavigator/Private/MapData/NavGridAdjacencyList.h"
#include "NavGridBlock.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FNavGridBlock
{
	GENERATED_BODY()
	
	FNavGridBlock() {}
	explicit FNavGridBlock(const FBox& NewBounds, const uint32 NewID) : Bounds(NewBounds), ID(NewID) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBox Bounds;

	FNavGridAdjacencyList Data;
	uint32 ID = 0;
};
