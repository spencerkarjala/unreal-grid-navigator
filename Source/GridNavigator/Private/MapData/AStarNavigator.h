#pragma once

#include "NavGridAdjacencyListTypes.h"

class FAStarNavigator
{
public:
	struct Node
	{
		Node* PrevNode;
		FVector Location;
		double GCost;
		NavGrid::EMapEdgeType InboundEdgeType;
	};
	
};
