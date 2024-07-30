#pragma once
#include <optional>

class FMapAdjacencyList
{
public:
	enum EMapEdgeType : uint8
	{
		None,
		Direct,
		Slope,
		SlopeBottom,
		SlopeTop,
		CliffUp,
		CliffDown
	};

	struct FEdge;
	
	struct FNode
	{
		typedef int64 ID;

		FNode()
			: X(0), Y(0), Layer(0), Height(0.0) {}
		FNode(const int32 NewX, const int32 NewY, const int32 NewLayer, const float NewHeight)
			: X(NewX), Y(NewY), Layer(NewLayer), Height(NewHeight) {}
		
		int32 X, Y, Layer;
		float Height;
		TArray<FEdge> OutEdges;

		friend FArchive& operator<<(FArchive& Ar, FNode& Rhs)
		{
			Ar << Rhs.X << Rhs.Y << Rhs.Layer << Rhs.Height << Rhs.OutEdges;
			return Ar;
		}

		bool operator==(const FNode& Rhs) const
		{
			return X == Rhs.X && Y == Rhs.Y && Layer == Rhs.Layer;
		}
	};
	
	struct FEdge
	{
		FEdge()
			: InID(0), OutID(0), Type(None), Direction(0, 0, 0) {}
		FEdge(const FNode::ID NewInID, const FNode::ID NewOutID, const EMapEdgeType NewType, const FVector& NewDirection)
			: InID(NewInID), OutID(NewOutID), Type(NewType), Direction(NewDirection) {}
		
		FNode::ID InID;
		FNode::ID OutID;
		EMapEdgeType Type;
		FVector Direction;

		FString ToString() const
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
	std::optional<std::reference_wrapper<const FMapAdjacencyList::FNode>> GetNode(const int64 X, const int64 Y, const int64 Layer);

	// TODO: upgrade to parameters
	static constexpr int AssumedMaxLayers = 16;
	static constexpr int AssumedMaxRows = 64;
};
