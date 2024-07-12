#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GNMappingServerSubsystem.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup=GridNavigator, meta=(BlueprintSpawnableComponent))
class GRIDNAVIGATOR_API UGNMappingServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/**
	 * @brief Tries to find a path from \p FromPoint to \p ToPoint on the grid.
	 * @param FromPoint Starting point to pathfind from
	 * @param ToPoint End point to pathfind to
	 * @param OutPoints Output array to place points in. Note: input data will be cleared
	 * @return true if operation completed successfully; false on error
	 */
	UFUNCTION(BlueprintCallable, Category="Pathfinding", meta=(ReturnDisplayName="PathingSuccess"))
	bool RequestPath(const FVector& FromPoint, const FVector& ToPoint, TArray<FVector>& OutPoints);
};
