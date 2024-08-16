#pragma once

#include <optional>

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

	void RemapFromBound(const UWorld& World, const FBox& Bound);
	TArray<FVector> FindPath(const FVector& From, const FVector& To);
	TPair<TArray<FVector>, TArray<FVector>> FindPath(const FVector& From, const FVector& To, const float DistanceBudget);
	FBoxSphereBounds GetBounds() const;

	TArray<NavGrid::FNode> GetMapNodeList();
	TArray<NavGrid::FEdge> GetMapEdgeList();
	std::optional<std::reference_wrapper<const NavGrid::FNode>> GetNode(const NavGrid::FNode::ID ID); 
	
	static FVector RoundToGrid(const FVector& Value);
	static FVector TruncToGrid(const FVector& Value);

	void Serialize(FArchive& Ar);
	
	FString Stringify();
	void DrawDebug(const UWorld& World);
	
	enum EMapEdgeType
	{
		None,
		Direct,
		Cliff,
		Slope,
	};

	static FIntVector2 WorldToGridIndex(const FVector2f& WorldCoord);
	static FIntVector3 WorldToGridIndex(const FVector& WorldCoord);
	static FVector2f GridIndexToWorld(const FIntVector2& IndexCoord);
	static FVector2f GridIndexToWorld(const FVector2f& IndexCoord);
	static FVector GridIndexToWorld(const FIntVector3& IndexCoord);
	static FVector GridIndexToWorld(const FVector& IndexCoord);
	static FVector2f SubGridIndexToWorld(const FVector2f& IndexCoord, FIntVector2 Direction, const float Alpha);

private:
	FMappingServer() {}

	void PopulateMap(const UWorld& World, const FBox& BoundingBox);

	NavGrid::FAdjacencyList Map;

	// TODO: make into customizable parameter; assume for now 100cm grid spacing
	static constexpr float ASSUMED_GRID_SPACING = 100.0;
};
