#include "NavSystemVisualizationLibrary.h"

#include "NavigationSystem.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavSystemVisualizationLibrary, Log, All);

const TArray<TObjectPtr<ANavigationData>>& UNavSystemVisualizationLibrary::GetNavigationData(TObjectPtr<UWorld> WorldInstance)
{
	if (!WorldInstance) {
		UE_LOG(LogNavSystemVisualizationLibrary, Error, TEXT("Received invalid world reference when trying to GetNavigationData"));
		return {};
	}

	const auto NavSys = Cast<UNavigationSystemV1>(WorldInstance->GetNavigationSystem());
	if (NavSys == nullptr) {
		UE_LOG(LogNavSystemVisualizationLibrary, Error, TEXT("Couldn't retrieve navigation system when trying to GetNavigationData"));
		return {};
	}

	return NavSys->NavDataSet;
}
