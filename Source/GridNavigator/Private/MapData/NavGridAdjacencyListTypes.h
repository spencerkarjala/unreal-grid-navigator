#pragma once

namespace NavGrid
{
	enum EMapEdgeType : uint8
	{
		None,
		Direct,
		Cliff,
		Slope,
		SlopeBottom,
		SlopeTop,
	};

	struct FEdge;
		
	struct FNode
	{
		typedef int64 ID;

		FNode();
		FNode(const int32 NewX, const int32 NewY, const int32 NewLayer, const float NewHeight);
			
		int32 X, Y, Layer;
		float Height;
		TArray<FEdge> OutEdges;

		// serialization/deserialization
		friend FArchive& operator<<(FArchive& Ar, FNode& Rhs)
		{
			Ar << Rhs.X << Rhs.Y << Rhs.Layer << Rhs.Height << Rhs.OutEdges;
			return Ar;
		}
		
		bool operator==(const FNode& Rhs) const;
	};
		
	struct FEdge
	{
		FEdge();
		FEdge(const FNode::ID NewInID, const FNode::ID NewOutID, const EMapEdgeType NewType, const FVector& NewDirection);
			
		FNode::ID InID;
		FNode::ID OutID;
		EMapEdgeType Type;
		FVector Direction;

		FString ToString() const;

		// serialization/deserialization
		friend FArchive& operator<<(FArchive& Ar, FEdge& Rhs)
		{
			Ar << Rhs.InID << Rhs.OutID;
				
			if (Ar.IsLoading()) {
				uint8 TypeAsInt;
				Ar << TypeAsInt;
				Rhs.Type = static_cast<EMapEdgeType>(TypeAsInt);
			}
			else if (Ar.IsSaving()) {
				uint8 TypeAsInt = Rhs.Type;
				Ar << TypeAsInt;
			}

			Ar << Rhs.Direction;
				
			return Ar;
		}
	};
}
