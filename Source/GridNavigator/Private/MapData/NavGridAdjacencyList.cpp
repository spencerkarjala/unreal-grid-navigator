#include "NavGridAdjacencyList.h"

#include <functional>
#include <optional>

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

bool FNavGridAdjacencyList::HasNode(const int QueryX, const int QueryY, const int QueryZ) const
{
	const NavGrid::FNode::ID id = this->GetNodeId(QueryX, QueryY, QueryZ);
	return Nodes.Contains(id);
}

void FNavGridAdjacencyList::AddNode(int X, int Y, int Z, float Height)
{
	this->Nodes.Emplace(GetNodeId(X, Y, Z), NavGrid::FNode(X, Y, Z, Height));
}

TArray<FVector> FNavGridAdjacencyList::GetReachableNeighbors(const int64 X, const int64 Y, const int64 Z) const
{
	const auto NodeResult = GetNode(X, Y, Z);
	if (!NodeResult.has_value()) {
		return {};
	}
	const auto Node = NodeResult->get();

	TArray<FVector> Result;
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

TArray<FVector> FNavGridAdjacencyList::FindPath(const FIntVector3& From, const FIntVector3& To) {
	const std::function Heuristic = [this](const FVector& Lhs, const FVector& Rhs) -> double
	{
		// ignore height since we only care about grid coordinates
		return FMath::Sqrt((Rhs.Y-Lhs.Y)*(Rhs.Y-Lhs.Y) + (Rhs.X-Lhs.X)*(Rhs.X-Lhs.X));
	};

	if (!this->HasNode(From.X, From.Y, From.Z) || !this->HasNode(To.X, To.Y, To.Z)) {
		return TArray<FVector>();
	}

	const auto StartNodeResult = GetNode(From.X, From.Y, From.Z);
	const auto FinalNodeResult = GetNode(To.X,   To.Y,   To.Z);

	if (!StartNodeResult.has_value() || !FinalNodeResult.has_value()) {
		return TArray<FVector>();
	}

	const auto StartNode = StartNodeResult->get();
	const auto FinalNode = FinalNodeResult->get();

	const FVector StartNodePos(StartNode.X, StartNode.Y, StartNode.Z);
	const FVector FinalNodePos(FinalNode.X, FinalNode.Y, FinalNode.Z);

	// perform actual pathfinding
	TAStarNavigator<FNavGridAdjacencyList, FVector> Navigator;
	Navigator.Heuristic = Heuristic;
	TArray<FVector> PathNodes = Navigator.Navigate(*this, StartNodePos, FinalNodePos);
	if (PathNodes.Num() == 0) {
		return PathNodes;
	}

	// adds halfway-between grid points to account for the cases where map
	// incline changes; ie. tops and bottoms of sloped floors
	TArray<FVector> UnfilteredPath;
	for (int i = 1; i < PathNodes.Num(); ++i) {
		const auto PointA = PathNodes[i-1];
		const auto PointB = PathNodes[i];

		UnfilteredPath.Add(PointA);

		const auto PointANodeResult = GetNode(PointA.X, PointA.Y, PointA.Z);
		check(PointANodeResult.has_value());
		const auto PointANode = PointANodeResult->get();

		const auto PointBID = GetNodeId(PointB.X, PointB.Y, PointB.Z);
		NavGrid::EMapEdgeType EdgeTypeAB = NavGrid::None;
		for (const auto& OutEdge : PointANode.OutEdges) {
			if (OutEdge.OutID == PointBID) {
				EdgeTypeAB = OutEdge.Type;
				break;
			}
		}

		if (EdgeTypeAB != NavGrid::EMapEdgeType::SlopeBottom && EdgeTypeAB != NavGrid::EMapEdgeType::SlopeTop) {
			continue;
		}
		
		FVector Midpoint = (PointA + PointB) / 2.0;
		const float MinHeight = FMath::Min(PointA.Z, PointB.Z);
		const float MaxHeight = FMath::Max(PointA.Z, PointB.Z);
		
		Midpoint.Z = (EdgeTypeAB == NavGrid::EMapEdgeType::SlopeBottom) ? MinHeight : MaxHeight;
		
		UnfilteredPath.Add(Midpoint);
	}
	
	if (PathNodes.Num() < 2) {
		return UnfilteredPath;
	}
	
	const auto FinalPoint = PathNodes.Last();
	UnfilteredPath.Add(FinalPoint);

	// filter path, removing unnecessary collinear points
	TArray<FVector> Path;
	FVector LineStart = UnfilteredPath[0];
	FVector LineEnd, TestPoint;
	
	Path.Add(UnfilteredPath[0]);
	for (int i = 2; i < UnfilteredPath.Num(); ++i) {
		LineEnd = UnfilteredPath[i];
		TestPoint = UnfilteredPath[i-1];
		
		UE::Geometry::FLine3d StartEndLine = UE::Geometry::FLine3d::FromPoints(LineStart, LineEnd);
		float DistanceToLine = StartEndLine.DistanceSquared(TestPoint);
		if (DistanceToLine >= UE_KINDA_SMALL_NUMBER) {
			Path.Add(TestPoint);
			LineStart = TestPoint;
		}
	}
	Path.Add(UnfilteredPath.Last());
	
	return Path;
}
#pragma optimize("", on)

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
