#pragma once
#include <optional>

#include "NavGridAdjacencyListTypes.h"

class FNavGridAdjacencyList
{
public:
	std::optional<std::reference_wrapper<const NavGrid::FNode>> GetNode(const int64 X, const int64 Y, const int64 Z) const;
	std::optional<std::reference_wrapper<const NavGrid::FNode>> GetNode(const NavGrid::FAdjacencyListIndex& Index) const;
	FORCEINLINE bool HasNode(const int X, const int Y, const int Z) const;
	FORCEINLINE bool HasNode(const NavGrid::FAdjacencyListIndex& Index) const;
	FORCEINLINE void AddNode(const int64 X, const int64 Y, const int64 Z, float Height);
	FORCEINLINE void AddNode(const NavGrid::FAdjacencyListIndex& Index, float Height);

	TArray<NavGrid::FAdjacencyListIndex> GetReachableNeighbors(const NavGrid::FAdjacencyListIndex& Index) const;
	
	TArray<NavGrid::FNode> GetNodeList();
	TArray<NavGrid::FEdge> GetEdgeList();
	
	void CreateEdge(const NavGrid::FAdjacencyListIndex& FromIndex, const float FromHeight, const NavGrid::FAdjacencyListIndex& ToIndex, const float ToHeight, const NavGrid::EMapEdgeType EdgeType);
	bool IsEdgeTraversable(const NavGrid::FEdge& Edge) const;
	
	void Clear();
	FString Stringify();

	void Serialize(FArchive& Archive);

private:
	TMap<NavGrid::FAdjacencyListIndex, NavGrid::FNode> Nodes;
};
