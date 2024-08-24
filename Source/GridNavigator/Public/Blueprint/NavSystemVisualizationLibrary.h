#pragma once

#include "CoreMinimal.h"
#include "NavSystemVisualizationLibrary.generated.h"

UCLASS(Blueprintable, MinimalAPI)
class UNavSystemVisualizationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	virtual void PostInitProperties() override;
	virtual void FinishDestroy() override;
	
	/**
	 * @brief Exposes access to NavigationData actors that are active in the navigation system.
	 * 
	 * @param WorldInstance A reference to the current World instance
	 * @param NavData The array to write navigation data to (expected to be empty)
	 * @return true if the data was retrieved successfully; false otherwise
	 *
	 * @note This should be used for reading and hooking into data updates only
	 */
	UFUNCTION(BlueprintCallable, Category = "Navigation", DisplayName = "Get Navigation Data")
	static GRIDNAVIGATOR_API bool GetNavigationData(const UWorld* WorldInstance, TArray<ANavigationData*>& NavData);
	
	static TObjectPtr<UWorld> SavedWorldInstance;
private:
	static void HandlePostWorldInitialization(UWorld* World, FWorldInitializationValues WorldInitValues);


	FDelegateHandle PostWorldInitDelegateHandle;
};
