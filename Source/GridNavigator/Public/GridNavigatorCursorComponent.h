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
	 * @brief Sets the endpoint position of the GridNavigatorCursor using the results from a raycast.
	 * @param HitResult The result of a raycast to the desired destination in the world
	 * @returns false if an error occurs while setting cursor position; true otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Cursor", meta=(DisplayName="UpdatePositionByRaycast"))
	bool UpdatePositionByRaycast(const FHitResult& HitResult);

	/**
	 * @brief Sets the endpoint position of the GridNavigatorCursor using a world destination vector.
	 * @param WorldDestination The vector to set the cursor endpoint to
	 * @param DestNormal The normal vector of the ground geometry at the `WorldDestination`
	 * @returns false if an error occurs while setting cursor position; true otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Cursor", meta=(DisplayName="UpdatePositionByDestination"))
	bool UpdatePositionByDestination(const FVector& WorldDestination, const FVector& DestNormal = FVector(0, 0, 1));

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> DestinationMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineMeshComponent> PathMeshComponent;

private:
	// todo: add these to some sort of config
	float TodoCosOfMaxInclineAngle = 0.70710678118; // cos(45.0 deg)

	FVector CurrCursorLocation = FVector(-99999.f);
};
