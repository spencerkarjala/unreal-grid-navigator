#pragma once

class FMapAdjacencyList
{
public:
	enum EMapEdgeType
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
		typedef int ID;
		
		FNode(int NewX, int NewY, int NewLayer, float NewHeight) : X(NewX), Y(NewY), Layer(NewLayer), Height(NewHeight) {}
		
		int X, Y, Layer;
		float Height;
		TArray<FEdge> OutEdges;
	};
	
	struct FEdge
	{
		FEdge(const FNode::ID NewInID, const FNode::ID NewOutID, const EMapEdgeType NewType) : InID(NewInID), OutID(NewOutID), Type(NewType) {}
		
		const FNode::ID InID;
		const FNode::ID OutID;
		EMapEdgeType Type;
	};
	
	TMap<FNode::ID, FNode> Nodes;

	bool HasNode(const int QueryX, const int QueryY, const int QueryLayer) const;
	void AddNode(int X, int Y, int Layer, float Height);
	void CreateEdge(int FromX, int FromY, int FromLayer, float FromHeight, int ToX, int ToY, int ToLayer, float ToHeight, EMapEdgeType EdgeType);
	void Clear();
	FString Stringify();
	void DrawDebug(const UWorld& World);
	TArray<FVector> FindPath(const FIntVector2& From, const FIntVector2& To);

private:
	static FNode::ID GetNodeId(int X, int Y, int Layer);
	static FNode::ID GetNodeId(const FNode& Node);
	FNode& GetNode(int X, int Y, int Layer);

	// TODO: upgrade to parameters
	static constexpr int AssumedMaxLayers = 1;
	static constexpr int AssumedMaxRows = 64;
};
