#pragma once

#include "CoreMinimal.h"

#include "GridNavigatorCursor.generated.h"

UCLASS()
class GRID_NAVIGATOR_API AGridNavigatorCursor : public AActor
{
	GENERATED_BODY()

public:
	AGridNavigatorCursor();

protected:
	virtual void BeginPlay() override;
};
