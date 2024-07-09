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

	UFUNCTION(BlueprintCallable, Category="Cursor")
	void UpdateVisibility(const bool NewVisibility);

	UFUNCTION(BlueprintCallable, Category="Cursor")
	void UpdatePosition(const FHitResult& HitResult);

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
