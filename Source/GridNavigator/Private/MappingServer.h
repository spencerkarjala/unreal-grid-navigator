#pragma once

#include "MapAdjacencyList.h"

class UWorld;

class FMappingServer
{
public:
	FMappingServer(const FMappingServer&) = delete;
	FMappingServer& operator=(const FMappingServer&) = delete;

	static FMappingServer& GetInstance()
	{
		static FMappingServer Instance;
		return Instance;
	}

	void RemapFromWorld(const UWorld& World);
	TArray<FVector> FindPath(const FVector& From, const FVector& To);
	TPair<TArray<FVector>, TArray<FVector>> FindPath(const FVector& From, const FVector& To, const float DistanceBudget);

	TArray<FMapAdjacencyList::FNode> GetMapNodeList();
	TArray<FMapAdjacencyList::FEdge> GetMapEdgeList();
	
	static FVector RoundToGrid(const FVector& Value);
	static FVector TruncToGrid(const FVector& Value);

	void Serialize(FArchive& Ar);
	
	FString Stringify();
	void DrawDebug(const UWorld& World);
	
	enum EMapEdgeType
	{
		None,
		Direct,
		Slope,
		CliffUp,
		CliffDown
	};

	static FIntVector2 WorldToGridIndex(const FVector2f& WorldCoord);
	static FVector2f GridIndexToWorld(const FVector2f& IndexCoord);
	static FVector2f SubGridIndexToWorld(const FVector2f& IndexCoord, FIntVector2 Direction, const float Alpha);

private:
	FMappingServer() {}

	void PopulateMap(const UWorld& World, const FBox& BoundingBox);

	FMapAdjacencyList Map;

	// TODO: make into customizable parameter; assume for now 64x64 grid
	static constexpr int ASSUMED_MAX_MAP_SIZE = 64;

	// TODO: make into customizable parameter; assume for now 100cm grid spacing
	static constexpr float ASSUMED_GRID_SPACING = 100.0;
};
