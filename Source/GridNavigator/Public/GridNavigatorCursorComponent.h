#pragma once

#include "CoreMinimal.h"
#include "GridNavigatorCursorComponent.generated.h"

class USplineMeshComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup=GridNavigator, meta=(BlueprintSpawnableComponent))
class GRIDNAVIGATOR_API UGridNavigatorCursorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridNavigatorCursorComponent();

	/**
	 * @brief Sets the endpoint position of the GridNavigatorCursor using a world destination vector.
	 * @param WorldDestination The vector to set the cursor endpoint to
	 * @param DestNormal The normal vector of the ground geometry at the `WorldDestination`
	 * @returns false if an error occurs while setting cursor position; true otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Cursor", meta=(ReturnDisplayName="Update Success"))
	bool UpdatePosition(const FVector& WorldDestination, const FVector& DestNormal = FVector(0, 0, 1));

	/**
	 * @brief Checks cursor position against a world position to see if it needs to be updated.
	 * @param WorldDestination
	 * @return true if new `WorldDestination` results in a cursor update; false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Cursor", meta=(ReturnDisplayName="Should Update"))
	bool ShouldUpdatePosition(const FVector& WorldDestination);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> DestinationMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineMeshComponent> PathMeshComponent;

private:
	// todo: add these to some sort of config
	const float TodoCosOfMaxInclineAngle = 0.70710678118; // cos(45.0 deg)
	const float TodoDistDeltaThreshold = 2e-4;

	FVector CurrCursorLocation = FVector(-99999.f);
};
