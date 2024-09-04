#pragma once
#include <optional>

#include "NavGridAdjacencyListTypes.h"

class FNavGridAdjacencyList
{
public:
	TMap<NavGrid::FNode::ID, NavGrid::FNode> Nodes;

	std::optional<std::reference_wrapper<const NavGrid::FNode>> GetNode(const int64 X, const int64 Y, const int64 Z) const;
	std::optional<std::reference_wrapper<const NavGrid::FNode>> GetNode(const FInt64Vector3& Index) const;
	bool HasNode(const int X, const int Y, const int Z) const;
	bool HasNode(const FInt64Vector3& Index) const;
	void AddNode(int X, int Y, int Z, float Height);

	TArray<FVector> GetReachableNeighbors(const int64 X, const int64 Y, const int64 Z) const;
	
	TArray<NavGrid::FNode> GetNodeList();
	TArray<NavGrid::FEdge> GetEdgeList();
	
	void CreateEdge(int FromX, int FromY, int FromZ, float FromHeight, int ToX, int ToY, int ToZ, float ToHeight, NavGrid::EMapEdgeType EdgeType);
	bool IsEdgeTraversable(const NavGrid::FEdge& Edge) const;
	
	void Clear();
	FString Stringify();
	void DrawDebug(const UWorld& World);
	TArray<FVector> FindPath(const FIntVector3& From, const FIntVector3& To);

private:
	static NavGrid::FNode::ID GetNodeId(const int64 X, const int64 Y, const int64 Z);
	static NavGrid::FNode::ID GetNodeId(const NavGrid::FNode& Node);
	
	// TODO: upgrade to parameters
	static constexpr int AssumedMaxLayers = 1024;
	static constexpr int AssumedMaxRows = 1024;
};
