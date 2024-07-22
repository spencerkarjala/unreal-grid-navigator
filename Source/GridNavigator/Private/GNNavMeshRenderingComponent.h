#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "GNNavMeshRenderingComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GRIDNAVIGATOR_API UGNNavMeshRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UGNNavMeshRenderingComponent() = default;
};
