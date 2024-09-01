#include "NavGridDataSerializer.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridDataSerializer, Log, All)

void operator<<(FArchive& Archive, FNavGridAdjacencyList& Data)
{
	Archive << Data.Nodes;
}

void operator<<(FArchive& Archive, FNavGridBlock& Data)
{
	Archive << Data.Bounds;
}

void operator<<(FArchive& Archive, FNavGridLevel& Data)
{
	Archive << Data.Blocks;
	Archive << Data.Map;
}

void FNavGridDataSerializer::Serialize(FArchive& Ar, ANavigationGridData* NavData)
{
	if (NavData == nullptr) {
		UE_LOG(LogNavGridDataSerializer, Error, TEXT("Tried to serialize a null NavigationGridData reference"));
		return;
	}
	if (!NavData->LevelData) {
		UE_LOG(LogNavGridDataSerializer, Error, TEXT("Tried to serialize a NavigationGridData object with no LevelData"));
		return;
	}

	Ar << *NavData->LevelData;
}
