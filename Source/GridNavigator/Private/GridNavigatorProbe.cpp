#include "GridNavigatorProbe.h"
#include "MappingServer.h"

DECLARE_LOG_CATEGORY_CLASS(LogGridNavigatorProbe, Log, All);

AGridNavigatorProbe::AGridNavigatorProbe()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridNavigatorProbe::BeginPlay()
{
	Super::BeginPlay();

	const auto* World = GetWorld();
	if (!IsValid(World)) {
		UE_LOG(LogGridNavigatorProbe, Error, TEXT("Tried to load GridNavigatorProbe without a valid world reference"));
		return;
	}

	FMappingServer::GetInstance().RemapFromWorld(*World);
}
