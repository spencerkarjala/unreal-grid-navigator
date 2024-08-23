#pragma once

#include "CoreMinimal.h"
#include "NavSystemVisualizationLibrary.generated.h"

UCLASS(Blueprintable, MinimalAPI)
class GRIDNAVIGATOR_API UNavSystemVisualizationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Navigation", DisplayName = "Get Navigation Data")
	static const TArray<TObjectPtr<ANavigationData>>& GetNavigationData(TObjectPtr<UWorld> WorldInstance);
};
