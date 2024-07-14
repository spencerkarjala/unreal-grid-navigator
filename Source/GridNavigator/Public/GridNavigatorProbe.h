#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridNavigatorProbe.generated.h"

UCLASS()
class GRIDNAVIGATOR_API AGridNavigatorProbe : public AActor
{
	GENERATED_BODY()

public:
	AGridNavigatorProbe();

protected:
	virtual void BeginPlay() override;
};
