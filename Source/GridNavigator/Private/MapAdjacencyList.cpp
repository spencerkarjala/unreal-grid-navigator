#include "MapAdjacencyList.h"
#include "PriorityQueue.h"

#include <functional>

DECLARE_LOG_CATEGORY_CLASS(LogMapAdjacencyList, Log, All)

bool FMapAdjacencyList::HasNode(const int QueryX, const int QueryY, const int QueryLayer) const
{
	const FNode::ID id = this->GetNodeId(QueryX, QueryY, QueryLayer);
	return Nodes.Contains(id);
}

void FMapAdjacencyList::AddNode(int X, int Y, int Layer, float Height)
{
	this->Nodes.Emplace(GetNodeId(X, Y, Layer), FNode(X, Y, Layer, Height));
}

void FMapAdjacencyList::CreateEdge(int FromX, int FromY, int FromLayer, float FromHeight, int ToX, int ToY, int ToLayer, float ToHeight, EMapEdgeType EdgeType)
{
	if (!this->HasNode(FromX, FromY, FromLayer)) {
		this->AddNode(FromX, FromY, FromLayer, FromHeight);
	}

	FNode::ID FromId = GetNodeId(FromX, FromY, FromLayer);
	FNode::ID ToId   = GetNodeId(ToX, ToY, ToLayer);

	Nodes[FromId].OutEdges.Emplace(FromId, ToId, EdgeType);
}

void FMapAdjacencyList::Clear()
{
	this->Nodes.Empty();
}

FString FMapAdjacencyList::Stringify()
{
	FString Output;

	Output.Append(TEXT("Map has structure:\r\n"));

	for (const auto& [Id, Node] : this->Nodes) {
		Output.Appendf(TEXT("\tID %d = (%d, %d) on layer %d and with outward edges:\r\n"), Id, Node.X, Node.Y, Node.Layer);
		for (const auto& [InId, OutId, Type] : Node.OutEdges) {
			const FNode& In = Nodes[InId];
			const FNode& Out = Nodes[OutId];
			Output.Appendf(
				TEXT("\t\tID %d: (%d, %d) L%d H%0.2f -> ID %d: (%d, %d)  L %d  H %0.2f\r\n"),
				InId,  In.X,  In.Y,  In.Layer,  In.Height,
				OutId, Out.X, Out.Y, Out.Layer, Out.Height
			);
		}
	}

	return Output;
}

void FMapAdjacencyList::DrawDebug(const UWorld& World)
{
	for (const auto& [Id, Node] : Nodes) {
		for (const auto& [ToId, FromId, EdgeType] : Node.OutEdges) {
			const auto& FromNode = Node;
			const auto& ToNode = Nodes[FromId];

			FColor DebugLineColor;
			switch(EdgeType) {
				case Direct:      DebugLineColor = FColor(0,   255, 255); break;
				case Slope:       DebugLineColor = FColor(0,   255, 0);   break;
				case SlopeBottom: DebugLineColor = FColor(255, 200, 0);   break;
				case SlopeTop:    DebugLineColor = FColor(200, 255, 0);   break;
				case CliffDown:
				case CliffUp:     DebugLineColor = FColor(0,   0,   255); break;
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
	FMapAdjacencyList::EMapEdgeType InboundEdgeType;
};

TArray<FVector> FMapAdjacencyList::FindPath(const FIntVector2& From, const FIntVector2& To) {
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

	if (!this->HasNode(From.X, From.Y, 0) || !this->HasNode(To.X, To.Y, 0)) {
		return TArray<FVector>();
	}

	const FNode& StartNode = this->GetNode(From.X, From.Y, 0);
	const FNode& EndNode   = this->GetNode(To.X,   To.Y,   0);

	FVector StartLocation(StartNode.X, StartNode.Y, StartNode.Height);
	FVector EndLocation  (EndNode.X,   EndNode.Y,   EndNode.Height);
	
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

		const FVector& CurrNodeLocation = CurrNodePtr->Location;
		const uint32 CurrNodeLayer = FMath::RoundToInt(CurrNodeLocation.Z / 25.0);
		const FNode& CurrNode = this->GetNode(CurrNodeLocation.X, CurrNodeLocation.Y, CurrNodeLayer);
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

			const FVector NeighborLocation = FVector(OutNode.X, OutNode.Y, OutNode.Height);

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

FMapAdjacencyList::FNode::ID FMapAdjacencyList::GetNodeId(const int64 X, const int64 Y, const int64 Layer)
{
	return (
		Layer * AssumedMaxRows * AssumedMaxLayers
		  + Y * AssumedMaxRows
		  + X
	);
}

FMapAdjacencyList::FNode::ID FMapAdjacencyList::GetNodeId(const FNode& Node)
{
	return GetNodeId(Node.X, Node.Y, Node.Layer);
}

FMapAdjacencyList::FNode& FMapAdjacencyList::GetNode(const int64 X, const int64 Y, const int64 Layer)
{
	const FNode::ID Id = this->GetNodeId(X, Y, Layer);
	check(this->Nodes.Contains(Id));
	return Nodes[Id];
}
