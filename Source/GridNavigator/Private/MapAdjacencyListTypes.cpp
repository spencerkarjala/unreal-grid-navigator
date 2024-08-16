#include "MapAdjacencyListTypes.h"

namespace NavGrid
{
	FNode::FNode()
		: X(0), Y(0), Layer(0), Height(0.0) {}
	FNode::FNode(const int32 NewX, const int32 NewY, const int32 NewLayer, const float NewHeight)
		: X(NewX), Y(NewY), Layer(NewLayer), Height(NewHeight) {}

	bool FNode::operator==(const FNode& Rhs) const
	{
		return X == Rhs.X && Y == Rhs.Y && Layer == Rhs.Layer;
	}
			
	FEdge::FEdge()
		: InID(0), OutID(0), Type(None), Direction(0, 0, 0) {}
	FEdge::FEdge(const FNode::ID NewInID, const FNode::ID NewOutID, const EMapEdgeType NewType, const FVector& NewDirection)
		: InID(NewInID), OutID(NewOutID), Type(NewType), Direction(NewDirection) {}
			
	FString FEdge::ToString() const
	{
		return FString::Printf(
			TEXT("Edge at %p, Edge in: %llu, Edge out: %llu, Edge Type: %d, Edge Direction: (%0.2f, %0.2f, %0.2f)"), 
			this,
			InID,
			OutID,
			Type,
			Direction.X,
			Direction.Y,
			Direction.Z
		);
	}
}
