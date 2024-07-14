#pragma once

#include "CoreMinimal.h"
#include "GridNavigatorCursorComponent.generated.h"

class USplineMeshComponent;
class USplineComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup=GridNavigator, meta=(BlueprintSpawnableComponent))
class GRIDNAVIGATOR_API UGridNavigatorCursorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridNavigatorCursorComponent();
	
	virtual void PostInitProperties() override;

	/**
	 * @brief Sets the endpoint position of the GridNavigatorCursor using a world destination vector.
	 * @param WorldDestination The path endpoint that the cursor highlights
	 * @param DestNormal The normal vector of the ground geometry at the \p WorldDestination
	 * @returns \b false if an error occurs while setting cursor position; \b true otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Cursor", meta=(ReturnDisplayName="Update Success"))
	bool UpdatePosition(const FVector& WorldDestination, const FVector& DestNormal = FVector(0, 0, 1));

	/**
	 * @brief Checks cursor position against a world position to see if it needs to be updated.
	 * @param WorldDestination The world position to use to see if the cursor needs to update
	 * @return \b true if new \p WorldDestination results in a cursor update; \b false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Cursor", meta=(ReturnDisplayName="Should Update"))
	bool ShouldUpdatePosition(const FVector& WorldDestination);

protected:	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> DestinationMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMesh> PathMeshBase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<USplineMeshComponent>> PathMeshComponents;

private:
	bool UpdatePathMesh(const TArray<FVector>& Points);
	
	// todo: add these to some sort of config
	const float TodoCosOfMaxInclineAngle = 0.70710678118; // cos(45.0 deg)
	const float TodoDistDeltaThreshold = 2e-4;

	FVector CurrCursorLocation = FVector(-99999.f);
};
