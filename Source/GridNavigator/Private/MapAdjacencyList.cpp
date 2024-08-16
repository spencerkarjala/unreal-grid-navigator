#include "MapAdjacencyList.h"
#include "PriorityQueue.h"

#include <functional>
#include <optional>

DECLARE_LOG_CATEGORY_CLASS(LogMapAdjacencyList, Log, All)

namespace NavGrid
{
	bool FAdjacencyList::HasNode(const int QueryX, const int QueryY, const int QueryLayer) const
	{
		const FNode::ID id = this->GetNodeId(QueryX, QueryY, QueryLayer);
		return Nodes.Contains(id);
	}

	void FAdjacencyList::AddNode(int X, int Y, int Layer, float Height)
	{
		this->Nodes.Emplace(GetNodeId(X, Y, Layer), FNode(X, Y, Layer, Height));
	}

	TArray<FNode> FAdjacencyList::GetNodeList()
	{
		TArray<FNode> Output;
		for (const auto& [ID, Node] : Nodes) {
			Output.Add(Node);
		}
		return Output;
	}

	TArray<FEdge> FAdjacencyList::GetEdgeList()
	{
		TArray<FEdge> Output;
		for (const auto [ID, Node] : Nodes) {
			Output.Append(Node.OutEdges);
		}
		return Output;
	}

	void FAdjacencyList::CreateEdge(int FromX, int FromY, int FromLayer, float FromHeight, int ToX, int ToY, int ToLayer, float ToHeight, EMapEdgeType EdgeType)
	{
		if (!this->HasNode(FromX, FromY, FromLayer)) {
			this->AddNode(FromX, FromY, FromLayer, FromHeight);
		}

		FNode::ID FromId = GetNodeId(FromX, FromY, FromLayer);
		FNode::ID ToId   = GetNodeId(ToX, ToY, ToLayer);

		const FVector Direction(ToX - FromX, ToY - FromY, ToLayer - FromLayer);

		Nodes[FromId].OutEdges.Emplace(FromId, ToId, EdgeType, Direction);
	}

	void FAdjacencyList::Clear()
	{
		this->Nodes.Empty();
	}

	FString FAdjacencyList::Stringify()
	{
		FString Output;

		Output.Append(TEXT("Map has structure:\r\n"));

		for (const auto& [Id, Node] : this->Nodes) {
			Output.Appendf(TEXT("\tID %d = (%d, %d) on layer %d and with outward edges:\r\n"), Id, Node.X, Node.Y, Node.Layer);
			for (const auto& [InId, OutId, Type, Direction] : Node.OutEdges) {
				const FNode& In = Nodes[InId];
				const FNode& Out = Nodes[OutId];
				Output.Appendf(
					TEXT("\t\tID %d: (%d, %d) L%d H%0.2f -> ID %d: (%d, %d)  L %d  H %0.2f | Dir: (%0.2f, %0.2f, %0.2f)\r\n"),
					InId,  In.X,  In.Y,  In.Layer,  In.Height,
					OutId, Out.X, Out.Y, Out.Layer, Out.Height,
					Direction.X, Direction.Y, Direction.Z
				);
			}
		}

		return Output;
	}

	void FAdjacencyList::DrawDebug(const UWorld& World)
	{
		for (const auto& [Id, Node] : Nodes) {
			for (const auto& [ToId, FromId, EdgeType, Direction] : Node.OutEdges) {
				const auto& FromNode = Node;
				const auto& ToNode = Nodes[FromId];

				FColor DebugLineColor;
				switch(EdgeType) {
				case Direct:      DebugLineColor = FColor(0,   255, 255); break;
				case Slope:       DebugLineColor = FColor(0,   255, 0);   break;
				case SlopeBottom: DebugLineColor = FColor(255, 200, 0);   break;
				case SlopeTop:    DebugLineColor = FColor(200, 255, 0);   break;
				case Cliff:       DebugLineColor = FColor(0,   0,   255); break;
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

	struct FAStarNode
	{
		FAStarNode* PrevNode;
		FVector Location;
		double GCost;
		EMapEdgeType InboundEdgeType;
	};

	TArray<FVector> FAdjacencyList::FindPath(const FIntVector3& From, const FIntVector3& To) {
		const std::function Heuristic = [this](const FVector& Lhs, const FVector& Rhs) -> double
		{
			// ignore height since we only care about grid coordinates
			const FVector LhsNoHeight(Lhs.X, Lhs.Y, 0.0);
			const FVector RhsNoHeight(Rhs.X, Rhs.Y, 0.0);
			return FVector::Distance(LhsNoHeight, RhsNoHeight);
		};

		TPriorityQueue<FAStarNode*> OpenSet;
		TMap<const FNode*, double> CostSoFar;
		TArray<FAStarNode*> NodesToFree;

		if (!this->HasNode(From.X, From.Y, From.Z) || !this->HasNode(To.X, To.Y, To.Z)) {
			return TArray<FVector>();
		}

		const auto StartNodeResult = GetNode(From.X, From.Y, From.Z);
		const auto EndNodeResult   = GetNode(To.X,   To.Y,   To.Z);

		if (!StartNodeResult.has_value() || !EndNodeResult.has_value()) {
			return TArray<FVector>();
		}

		const auto& StartNode = StartNodeResult->get();
		const auto& EndNode   = EndNodeResult->get();

		FVector StartLocation(StartNode.X, StartNode.Y, StartNode.Layer);
		FVector EndLocation  (EndNode.X,   EndNode.Y,   EndNode.Layer);
		
		FAStarNode* FromAStarNode = new FAStarNode({ nullptr, StartLocation, 0.0, EMapEdgeType::None });
		NodesToFree.Push(FromAStarNode);
		OpenSet.Push(FromAStarNode, 0);
		CostSoFar.Add(&StartNode, 0.0);
		
		bool IsPathingSuccessful = false;
		FAStarNode* CurrNodePtr = nullptr;
		while (!OpenSet.IsEmpty()) {
			CurrNodePtr = OpenSet.Pop();
			const auto [PrevNodePtr, CurrLocation, CurrCost, CurrInboundEdgeType] = *CurrNodePtr;

			check(CurrNodePtr != nullptr);
		
			if (CurrLocation == EndLocation) {
				IsPathingSuccessful = true;
				break;
			}

			const auto CurrNodeResult = this->GetNode(CurrLocation.X, CurrLocation.Y, CurrLocation.Z);
			if (!CurrNodeResult.has_value()) {
				continue;
			}
			const auto& CurrNode = CurrNodeResult->get();
			const TArray<FEdge>& CurrOutwardEdges = CurrNode.OutEdges;

			for (const auto& Edge : CurrOutwardEdges) {
				if (Edge.Type != Direct && Edge.Type != Slope && Edge.Type != SlopeBottom && Edge.Type != SlopeTop) {
					continue;
				}

				if (!Nodes.Contains(Edge.InID)) {
					UE_LOG(LogMapAdjacencyList, Error, TEXT("FindPath failed; no input node %lld found for edge (%s)"), Edge.InID, *Edge.ToString());
					continue;
				}
				if (!Nodes.Contains(Edge.OutID)) {
					UE_LOG(LogMapAdjacencyList, Error, TEXT("FindPath failed; no output node %lld found for edge (%s)"), Edge.OutID, *Edge.ToString());
					continue;
				}
				
				const FNode& InNode  = this->Nodes[Edge.InID];
				const FNode& OutNode = this->Nodes[Edge.OutID];

				check(&CurrNode == &InNode);

				const FVector NeighborLocation = FVector(OutNode.X, OutNode.Y, OutNode.Layer);

				const double NeighborCost = CurrCost + Heuristic(CurrLocation, NeighborLocation);

				if (!CostSoFar.Contains(&OutNode) || NeighborCost < CostSoFar[&OutNode]) {
					FAStarNode* NeighborAStarNode = new FAStarNode({ CurrNodePtr, NeighborLocation, NeighborCost, Edge.Type });
					NodesToFree.Push(NeighborAStarNode);
					
					CostSoFar.Add(&OutNode, NeighborCost);
					const double Priority    = NeighborCost + Heuristic(NeighborLocation, EndLocation);
					NeighborAStarNode->GCost = Priority;
					OpenSet.Push(NeighborAStarNode, Priority);
				}
			}
		}

		TArray<TPair<FVector, EMapEdgeType>> PathWithEdgeTypes;
		if (IsPathingSuccessful) {
			while (CurrNodePtr != nullptr) {
				const auto [PrevNodePtr, CurrLocation, CurrCost, InboundEdgeType] = *CurrNodePtr;
				const FVector NewPathNode(CurrLocation.X, CurrLocation.Y, CurrLocation.Z);
				PathWithEdgeTypes.Push({ NewPathNode, InboundEdgeType });
				CurrNodePtr = CurrNodePtr->PrevNode;
			}
			Algo::Reverse(PathWithEdgeTypes);
		}

		for (const FAStarNode* NodePtr : NodesToFree) {
			delete NodePtr;
		}

		// adds halfway-between grid points to account for the cases where map
		// incline changes; ie. tops and bottoms of sloped floors
		TArray<FVector> UnfilteredPath;
		for (int i = 1; i < PathWithEdgeTypes.Num(); ++i) {
			const auto [PointA, ___]        = PathWithEdgeTypes[i-1];
			const auto [PointB, EdgeTypeAB] = PathWithEdgeTypes[i];

			UnfilteredPath.Add(PointA);

			if (EdgeTypeAB != SlopeBottom && EdgeTypeAB != SlopeTop) {
				continue;
			}
			
			FVector Midpoint = (PointA + PointB) / 2.0;
			const float MinHeight = FMath::Min(PointA.Z, PointB.Z);
			const float MaxHeight = FMath::Max(PointA.Z, PointB.Z);

			Midpoint.Z = (EdgeTypeAB == SlopeBottom) ? MinHeight : MaxHeight;

			UnfilteredPath.Add(Midpoint);
		}
		
		if (PathWithEdgeTypes.Num() < 2) {
			return UnfilteredPath;
		}
		
		const auto [FinalPoint, ___] = PathWithEdgeTypes.Last();
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

	FNode::ID FAdjacencyList::GetNodeId(const int64 X, const int64 Y, const int64 Layer)
	{
		return (
			Layer * AssumedMaxRows * AssumedMaxLayers
			  + Y * AssumedMaxRows
			  + X
		);
	}

	FNode::ID FAdjacencyList::GetNodeId(const FNode& Node)
	{
		return GetNodeId(Node.X, Node.Y, Node.Layer);
	}

	std::optional<std::reference_wrapper<const FNode>> FAdjacencyList::GetNode(const int64 X, const int64 Y, const int64 Layer)
	{
		const FNode::ID Id = GetNodeId(X, Y, Layer);
		if (!Nodes.Contains(Id)) {
			return std::nullopt;
		}
		return Nodes[Id];
	}
}