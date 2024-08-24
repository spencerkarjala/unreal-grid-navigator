#include "NavSystemVisualizationLibrary.h"

#include "NavigationSystem.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavSystemVisualizationLibrary, Log, All);

TObjectPtr<UWorld> UNavSystemVisualizationLibrary::SavedWorldInstance = nullptr;

void UNavSystemVisualizationLibrary::PostInitProperties()
{
	Super::PostInitProperties();

	PostWorldInitDelegateHandle = FWorldDelegates::OnPostWorldInitialization.AddStatic(&HandlePostWorldInitialization);
}

void UNavSystemVisualizationLibrary::FinishDestroy()
{
	Super::FinishDestroy();

	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitDelegateHandle);
}

bool UNavSystemVisualizationLibrary::GetNavigationData(const UWorld* WorldInstance, TArray<ANavigationData*>& NavData)
{
	const auto* NavSysBase = WorldInstance ? WorldInstance->GetNavigationSystem() : nullptr;
	const auto* NavSys = Cast<UNavigationSystemV1>(NavSysBase);
	if (!NavSys) {
		UE_LOG(LogNavSystemVisualizationLibrary, Error, TEXT("Failed to retrieve navigation system when trying to GetNavigationData"));
		return false;
	}

	for (auto DataItem : NavSys->NavDataSet) {
		NavData.Push(DataItem);
	}
	return true;
}

void UNavSystemVisualizationLibrary::HandlePostWorldInitialization(UWorld* World, FWorldInitializationValues WorldInitValues)
{
	SavedWorldInstance = World;
	if (!SavedWorldInstance) {
		UE_LOG(LogNavSystemVisualizationLibrary, Error, TEXT("Received invalid world refrence when trying to HandlePostWorldInitialization"));
		return;
	}
	
	const auto* NavSys = SavedWorldInstance->GetNavigationSystem();
	if (NavSys == nullptr) {
		UE_LOG(LogNavSystemVisualizationLibrary, Error, TEXT("Failed to retrieve navigation system when trying to HandlePostWorldInitialization"));
		return;
	}

	
}
