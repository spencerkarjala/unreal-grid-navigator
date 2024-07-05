#pragma once

#include "CoreMinimal.h"
#include "GridNavigatorCursor.generated.h"

class USplineMeshComponent;

UCLASS()
class GRIDNAVIGATOR_API AGridNavigatorCursor : public AActor
{
	GENERATED_BODY()

public:
	AGridNavigatorCursor();

	void SetVisibility(const bool bIsVisible);
	void UpdatePosition(const FVector& NewWorldPosition, const FRotator& NewOrientation);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> DestinationMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineMeshComponent> PathMeshComponent;
};
