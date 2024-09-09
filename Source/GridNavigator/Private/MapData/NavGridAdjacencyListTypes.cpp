#include "NavGridAdjacencyListTypes.h"

namespace NavGrid
{
	FNode::FNode() : Index(0), Height(0.0) {}
	FNode::FNode(const FAdjacencyListIndex& NewIndex, const float NewHeight) : Index(NewIndex), Height(NewHeight) {}

	bool FNode::operator==(const FNode& Rhs) const
	{
		return Index.X == Rhs.Index.X && Index.Y == Rhs.Index.Y && Index.Z == Rhs.Index.Z;
	}
			
	FEdge::FEdge() : InIndex(0), OutIndex(0), Type(None), Direction(0, 0, 0) {}
	FEdge::FEdge(const FAdjacencyListIndex& NewInIndex, const FAdjacencyListIndex& NewOutIndex, const EMapEdgeType NewType, const FVector& NewDirection)
		: InIndex(NewInIndex), OutIndex(NewOutIndex), Type(NewType), Direction(NewDirection) {}
			
	FString FEdge::ToString() const
	{
		return FString::Printf(
			TEXT("Edge at %p, Edge in: (%lld, %lld, %lld), Edge out: (%lld, %lld, %lld), Edge Type: %d, Edge Direction: (%0.2f, %0.2f, %0.2f)"), 
			this,
			InIndex.X, InIndex.Y, InIndex.Z,
			OutIndex.X, OutIndex.Y, OutIndex.Z,
			Type,
			Direction.X,
			Direction.Y,
			Direction.Z
		);
	}
}
