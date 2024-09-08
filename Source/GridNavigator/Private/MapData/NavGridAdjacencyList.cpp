#include "NavGridAdjacencyList.h"

#include <functional>
#include <optional>

#include "GridNavigatorConfig.h"
#include "Navigation/AStarNavigator.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridAdjacencyList, Log, All)

std::optional<std::reference_wrapper<const NavGrid::FNode>> FNavGridAdjacencyList::GetNode(const int64 X, const int64 Y, const int64 Z) const
{
	const NavGrid::FNode::ID Id = GetNodeId(X, Y, Z);
	if (!Nodes.Contains(Id)) {
		return std::nullopt;
	}
	return Nodes[Id];
}

std::optional<std::reference_wrapper<const NavGrid::FNode>> FNavGridAdjacencyList::GetNode(const FInt64Vector3& Index) const
{
	const NavGrid::FNode::ID ID = GetNodeId(Index.X, Index.Y, Index.Z);
	if (!Nodes.Contains(ID)) {
		return std::nullopt;
	}
	return Nodes[ID];
}

bool FNavGridAdjacencyList::HasNode(const int X, const int Y, const int Z) const
{
	const NavGrid::FNode::ID ID = this->GetNodeId(X, Y, Z);
	return Nodes.Contains(ID);
}

bool FNavGridAdjacencyList::HasNode(const FInt64Vector3& Index) const
{
	const NavGrid::FNode::ID ID = this->GetNodeId(Index.X, Index.Y, Index.Z);
	return Nodes.Contains(ID);
}

void FNavGridAdjacencyList::AddNode(int X, int Y, int Z, float Height)
{
	this->Nodes.Emplace(GetNodeId(X, Y, Z), NavGrid::FNode(X, Y, Z, Height));
}

TArray<FInt64Vector3> FNavGridAdjacencyList::GetReachableNeighbors(const FInt64Vector3& Index) const
{
	const auto NodeResult = GetNode(Index);
	if (!NodeResult.has_value()) {
		return {};
	}
	const auto Node = NodeResult->get();

	TArray<FInt64Vector3> Result;
	for (const auto& Edge : Node.OutEdges) {
		if (Edge.Type != NavGrid::Direct && Edge.Type != NavGrid::Slope && Edge.Type != NavGrid::SlopeBottom && Edge.Type != NavGrid::SlopeTop) {
			continue;
		}
		const auto& Neighbor = Nodes[Edge.OutID];
		Result.Emplace(Neighbor.X, Neighbor.Y, Neighbor.Z);
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

void FNavGridAdjacencyList::CreateEdge(int FromX, int FromY, int FromZ, float FromHeight, int ToX, int ToY, int ToZ, float ToHeight, NavGrid::EMapEdgeType EdgeType)
{
	if (!this->HasNode(FromX, FromY, FromZ)) {
		this->AddNode(FromX, FromY, FromZ, FromHeight);
	}

	NavGrid::FNode::ID FromId = GetNodeId(FromX, FromY, FromZ);
	NavGrid::FNode::ID ToId   = GetNodeId(ToX, ToY, ToZ);

	const FVector Direction(ToX - FromX, ToY - FromY, ToZ - FromZ);

	Nodes[FromId].OutEdges.Emplace(FromId, ToId, EdgeType, Direction);
}

bool FNavGridAdjacencyList::IsEdgeTraversable(const NavGrid::FEdge& Edge) const
{
	const bool IsTraversableType = Edge.Type == NavGrid::Direct || Edge.Type == NavGrid::Slope || Edge.Type == NavGrid::SlopeBottom || Edge.Type == NavGrid::SlopeTop;
	const bool IsSourceNodeValid = Nodes.Contains(Edge.InID);
	const bool IsTargetNodeValid = Nodes.Contains(Edge.OutID);

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

	for (const auto& [Id, Node] : this->Nodes) {
		Output.Appendf(TEXT("\tID %d = (%d, %d) on layer %d and with outward edges:\r\n"), Id, Node.X, Node.Y, Node.Z);
		for (const auto& [InId, OutId, Type, Direction] : Node.OutEdges) {
			const NavGrid::FNode& In = Nodes[InId];
			const NavGrid::FNode& Out = Nodes[OutId];
			Output.Appendf(
				TEXT("\t\tID %d: (%d, %d) L%d H%0.2f -> ID %d: (%d, %d)  L %d  H %0.2f | Dir: (%0.2f, %0.2f, %0.2f)\r\n"),
				InId,  In.X,  In.Y,  In.Z,  In.Height,
				OutId, Out.X, Out.Y, Out.Z, Out.Height,
				Direction.X, Direction.Y, Direction.Z
			);
		}
	}

	return Output;
}

void FNavGridAdjacencyList::DrawDebug(const UWorld& World)
{
	for (const auto& [Id, Node] : Nodes) {
		for (const auto& [ToId, FromId, EdgeType, Direction] : Node.OutEdges) {
			const auto& FromNode = Node;
			const auto& ToNode = Nodes[FromId];

			FColor DebugLineColor;
			switch(EdgeType) {
			case NavGrid::EMapEdgeType::Direct:      DebugLineColor = FColor(0,   255, 255); break;
			case NavGrid::EMapEdgeType::Slope:       DebugLineColor = FColor(0,   255, 0);   break;
			case NavGrid::EMapEdgeType::SlopeBottom: DebugLineColor = FColor(255, 200, 0);   break;
			case NavGrid::EMapEdgeType::SlopeTop:    DebugLineColor = FColor(200, 255, 0);   break;
			case NavGrid::EMapEdgeType::Cliff:       DebugLineColor = FColor(0,   0,   255); break;
			default:          DebugLineColor = FColor(255, 0,   0);   break;
			}

			DrawDebugLine(
				&World,
				FVector(FromNode.X * 100.0, FromNode.Y * 100.0, FromNode.Height),
				FVector(ToNode.X   * 100.0, ToNode.Y   * 100.0, ToNode.Height),
				DebugLineColor,
				true
			);
		}
	}
}

void FNavGridAdjacencyList::Serialize(FArchive& Archive)
{
	Archive << Nodes;
}

NavGrid::FNode::ID FNavGridAdjacencyList::GetNodeId(const int64 X, const int64 Y, const int64 Z)
{
	return (
		Z * AssumedMaxRows * AssumedMaxLayers
		  + Y * AssumedMaxRows
		  + X
	);
}

NavGrid::FNode::ID FNavGridAdjacencyList::GetNodeId(const NavGrid::FNode& Node)
{
	return GetNodeId(Node.X, Node.Y, Node.Z);
}
