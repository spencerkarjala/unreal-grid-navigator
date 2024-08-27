#pragma once
#include <optional>

#include "NavGridAdjacencyListTypes.h"

class FNavGridAdjacencyList
{
public:
	TMap<NavGrid::FNode::ID, NavGrid::FNode> Nodes;

	bool HasNode(const int QueryX, const int QueryY, const int QueryLayer) const;
	void AddNode(int X, int Y, int Layer, float Height);
	TArray<NavGrid::FNode> GetNodeList();
	TArray<NavGrid::FEdge> GetEdgeList();
	void CreateEdge(int FromX, int FromY, int FromLayer, float FromHeight, int ToX, int ToY, int ToLayer, float ToHeight, NavGrid::EMapEdgeType EdgeType);
	void Clear();
	FString Stringify();
	void DrawDebug(const UWorld& World);
	TArray<FVector> FindPath(const FIntVector3& From, const FIntVector3& To);

private:
	static NavGrid::FNode::ID GetNodeId(const int64 X, const int64 Y, const int64 Layer);
	static NavGrid::FNode::ID GetNodeId(const NavGrid::FNode& Node);
	std::optional<std::reference_wrapper<const NavGrid::FNode>> GetNode(const int64 X, const int64 Y, const int64 Layer);

	// TODO: upgrade to parameters
	static constexpr int AssumedMaxLayers = 16;
	static constexpr int AssumedMaxRows = 64;
};
