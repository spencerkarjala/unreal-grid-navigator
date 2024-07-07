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

	void SetVisibility(const bool bIsVisible);
	void UpdatePosition(const FVector& NewWorldPosition, const FRotator& NewOrientation);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> DestinationMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Mesh, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineMeshComponent> PathMeshComponent;
};
