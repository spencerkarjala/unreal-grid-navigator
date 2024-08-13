#pragma once
#include <optional>

#include "MapAdjacencyListTypes.h"

namespace NavigationMap
{
	class FAdjacencyList
	{
	public:
		TMap<FNode::ID, FNode> Nodes;

		bool HasNode(const int QueryX, const int QueryY, const int QueryLayer) const;
		void AddNode(int X, int Y, int Layer, float Height);
		TArray<FNode> GetNodeList();
		TArray<FEdge> GetEdgeList();
		void CreateEdge(int FromX, int FromY, int FromLayer, float FromHeight, int ToX, int ToY, int ToLayer, float ToHeight, EMapEdgeType EdgeType);
		void Clear();
		FString Stringify();
		void DrawDebug(const UWorld& World);
		TArray<FVector> FindPath(const FIntVector3& From, const FIntVector3& To);

	private:
		static FNode::ID GetNodeId(const int64 X, const int64 Y, const int64 Layer);
		static FNode::ID GetNodeId(const FNode& Node);
		std::optional<std::reference_wrapper<const FNode>> GetNode(const int64 X, const int64 Y, const int64 Layer);

		// TODO: upgrade to parameters
		static constexpr int AssumedMaxLayers = 16;
		static constexpr int AssumedMaxRows = 64;
	};
}
