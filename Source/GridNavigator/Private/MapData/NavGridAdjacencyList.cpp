#include "NavGridAdjacencyList.h"

#include <functional>
#include <optional>

#include "GridNavigatorConfig.h"
#include "Navigation/AStarNavigator.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridAdjacencyList, Log, All)

using NavGrid::FAdjacencyListIndex;

std::optional<std::reference_wrapper<const NavGrid::FNode>> FNavGridAdjacencyList::GetNode(const int64 X, const int64 Y, const int64 Z) const
{
	const FAdjacencyListIndex Index(X, Y, Z);
	if (!Nodes.Contains(Index)) {
		return std::nullopt;
	}
	return Nodes[Index];
}

std::optional<std::reference_wrapper<const NavGrid::FNode>> FNavGridAdjacencyList::GetNode(const FAdjacencyListIndex& Index) const
{
	if (!Nodes.Contains(Index)) {
		return std::nullopt;
	}
	return Nodes[Index];
}

bool FNavGridAdjacencyList::HasNode(const int X, const int Y, const int Z) const
{
	const FAdjacencyListIndex Index(X, Y, Z);
	return Nodes.Contains(Index);
}

bool FNavGridAdjacencyList::HasNode(const FAdjacencyListIndex& Index) const
{
	return Nodes.Contains(Index);
}

void FNavGridAdjacencyList::AddNode(const int64 X, const int64 Y, const int64 Z, const float Height)
{
	const FAdjacencyListIndex Index(X, Y, Z);
	Nodes.Emplace(Index, NavGrid::FNode(Index, Height));
}

void FNavGridAdjacencyList::AddNode(const FAdjacencyListIndex& Index, const float Height)
{
	Nodes.Emplace(Index, NavGrid::FNode(Index, Height));
}

TArray<FAdjacencyListIndex> FNavGridAdjacencyList::GetReachableNeighbors(const FAdjacencyListIndex& Index) const
{
	const auto NodeResult = GetNode(Index);
	if (!NodeResult.has_value()) {
		return {};
	}
	const auto Node = NodeResult->get();

	TArray<FAdjacencyListIndex> Result;
	for (const auto& Edge : Node.OutEdges) {
		if (Edge.Type != NavGrid::Direct && Edge.Type != NavGrid::Slope && Edge.Type != NavGrid::SlopeBottom && Edge.Type != NavGrid::SlopeTop) {
			continue;
		}
		const auto& Neighbor = Nodes[Edge.InIndex];
		Result.Emplace(Neighbor.Index);
	}
	return Result;
}

TArray<NavGrid::FNode> FNavGridAdjacencyList::GetNodeList()
{
	TArray<NavGrid::FNode> Output;
	for (const auto& [ID, Node] : Nodes) {
		Output.Add(Node);
	}
	return Output;
}

TArray<NavGrid::FEdge> FNavGridAdjacencyList::GetEdgeList()
{
	TArray<NavGrid::FEdge> Output;
	for (const auto [ID, Node] : Nodes) {
		Output.Append(Node.OutEdges);
	}
	return Output;
}

void FNavGridAdjacencyList::CreateEdge(const FAdjacencyListIndex& FromIndex, const float FromHeight, const FAdjacencyListIndex& ToIndex, const float ToHeight, const NavGrid::EMapEdgeType EdgeType)
{
	if (!HasNode(FromIndex)) {
		AddNode(FromIndex, FromHeight);
	}
	if (!HasNode(ToIndex)) {
		AddNode(ToIndex, ToHeight);
	}

	const FVector Direction(ToIndex.X - FromIndex.X, ToIndex.Y - FromIndex.Y, ToIndex.Z - FromIndex.Z);

	Nodes[FromIndex].OutEdges.Emplace(FromIndex, ToIndex, EdgeType, Direction);
}

bool FNavGridAdjacencyList::IsEdgeTraversable(const NavGrid::FEdge& Edge) const
{
	const bool IsTraversableType = Edge.Type == NavGrid::Direct || Edge.Type == NavGrid::Slope || Edge.Type == NavGrid::SlopeBottom || Edge.Type == NavGrid::SlopeTop;
	const bool IsSourceNodeValid = Nodes.Contains(Edge.InIndex);
	const bool IsTargetNodeValid = Nodes.Contains(Edge.OutIndex);

	return IsTraversableType && IsSourceNodeValid && IsTargetNodeValid;
}

void FNavGridAdjacencyList::Clear()
{
	this->Nodes.Empty();
}

FString FNavGridAdjacencyList::Stringify()
{
	FString Output;

	Output.Append(TEXT("Map has structure:\r\n"));

	for (const auto& [Index, Node] : Nodes) {
		Output.Appendf(TEXT("\tNode (%lld, %lld, %lld) has outward edges:\r\n"), Node.Index.X, Node.Index.Y, Node.Index.Z);
		for (const auto& [InIndex, OutIndex, Type, Direction] : Node.OutEdges) {
			Output.Appendf(
				TEXT("\t\tEdge from (%lld, %lld, %lld) to (%lld, %lld, %lld) | Dir: (%0.2f, %0.2f, %0.2f)\r\n"),
				InIndex.X,  InIndex.Y,  InIndex.Z,
				OutIndex.X, OutIndex.Y, OutIndex.Z,
				Direction.X, Direction.Y, Direction.Z
			);
		}
	}

	return Output;
}

void FNavGridAdjacencyList::Serialize(FArchive& Archive)
{
	Archive << Nodes;
}
