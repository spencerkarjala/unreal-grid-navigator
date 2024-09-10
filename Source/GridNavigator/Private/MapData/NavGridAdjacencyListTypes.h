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

	typedef FInt64Vector3 FAdjacencyListIndex;
		
	struct FNode
	{
		FNode();
		explicit FNode(const FAdjacencyListIndex& NewIndex);
			
		FAdjacencyListIndex Index;
		TArray<FEdge> OutEdges;

		// serialization/deserialization
		friend FArchive& operator<<(FArchive& Ar, FNode& Rhs)
		{
			Ar << Rhs.Index.X << Rhs.Index.Y << Rhs.Index.Z << Rhs.OutEdges;
			return Ar;
		}
		
		bool operator==(const FNode& Rhs) const;
	};
		
	struct FEdge
	{
		FEdge();
		FEdge(const FAdjacencyListIndex& NewInID, const FAdjacencyListIndex& NewOutID, const EMapEdgeType NewType, const FVector& NewDirection);
			
		FAdjacencyListIndex InIndex;
		FAdjacencyListIndex OutIndex;
		EMapEdgeType Type;
		FVector Direction;

		FString ToString() const;

		// serialization/deserialization
		friend FArchive& operator<<(FArchive& Ar, FEdge& Rhs)
		{
			Ar << Rhs.InIndex << Rhs.OutIndex;
				
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
