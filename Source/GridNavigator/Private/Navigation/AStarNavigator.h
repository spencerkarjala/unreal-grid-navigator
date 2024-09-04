#pragma once

#include "NavGridAdjacencyListTypes.h"

#include <functional>

#include "PriorityQueue.h"

template <typename MapT, typename LocationT>
concept can_query_nodes = requires(MapT Map, int64 X, int64 Y, int64 Z) {
	{ Map.HasNode(X, Y, Z) } -> std::convertible_to<bool>;
	{ Map.GetReachableNeighbors(X, Y, Z) } -> std::convertible_to<TArray<LocationT>>;
};

template <typename MapT, typename LocationT>
requires can_query_nodes<MapT, LocationT>
class TAStarNavigator
{
	struct FAStarNode
	{
		FAStarNode* PrevNode;
		FVector Location;
		double GCost;
	};
	
public:
	std::function<double(const LocationT& Lhs, const LocationT& Rhs)> Heuristic;

	/**
	 * @brief Performs A* navigation between two points on a provided map.
	 *
	 * @tparam MapT The map type, which must satisfy the can_query_nodes constraint
	 * @tparam LocationT The type representing locations within the map 
	 * 
	 * @param Map The map on which navigation is performed. The map type must support checking whether a
	 * particular node is present in the map, and fetching all neighbors that are reachable from that node
	 * within the map.
	 * @param StartLocation The starting location for pathfinding. 
	 * @param FinalLocation The target location for pathfinding.
	 * 
	 * @return An array of location values that make up a path from StartLocation to FinalLocation in Map.
	 *
	 * @pre The `Heuristic` parameter must be set before calling this function. This function determines
	 * the cost to travel between two locations and must be an admissible heuristic for A* to work.
	 */
	TArray<LocationT> Navigate(const MapT& Map, const LocationT& StartLocation, const LocationT& FinalLocation)
	{
		TPriorityQueue<FAStarNode*> OpenSet;
		TMap<LocationT, double> CostSoFar;
		TArray<FAStarNode*> NodesToFree;
		
		FAStarNode* FromAStarNode = new FAStarNode({ nullptr, StartLocation, 0.0 });
		NodesToFree.Push(FromAStarNode);
		OpenSet.Push(FromAStarNode, 0);
		CostSoFar.Add(StartLocation, 0.0);
		
		FAStarNode* CurrNodePtr = nullptr;
		bool IsNavigationSuccessful = false;
		while (!OpenSet.IsEmpty()) {
			CurrNodePtr = OpenSet.Pop();
			const auto [PrevNodePtr, CurrLocation, CurrCost] = *CurrNodePtr;

			check(CurrNodePtr != nullptr);
		
			if (CurrLocation == FinalLocation) {
				IsNavigationSuccessful = true;
				break;
			}

			if (!Map.HasNode(CurrLocation.X, CurrLocation.Y, CurrLocation.Z)) {
				continue;
			}
			
			const auto& Neighbors = Map.GetReachableNeighbors(CurrLocation.X, CurrLocation.Y, CurrLocation.Z);

			for (const auto& NeighborLocation : Neighbors) {
				const double NeighborCost = CurrCost + Heuristic(CurrLocation, NeighborLocation);

				if (!CostSoFar.Contains(NeighborLocation) || NeighborCost < CostSoFar[NeighborLocation]) {
					FAStarNode* NeighborAStarNode = new FAStarNode({ CurrNodePtr, NeighborLocation, NeighborCost });
					NodesToFree.Push(NeighborAStarNode);

					CostSoFar.Add(NeighborLocation, NeighborCost);
					const double Priority    = NeighborCost + Heuristic(NeighborLocation, FinalLocation);
					NeighborAStarNode->GCost = Priority;
					OpenSet.Push(NeighborAStarNode, Priority);
				}
			}
		}

		TArray<LocationT> Result;
		if (IsNavigationSuccessful) {
			while (CurrNodePtr != nullptr) {
				const auto [PrevNodePtr, CurrLocation, CurrCost] = *CurrNodePtr;
				const FVector NewPathNode(CurrLocation.X, CurrLocation.Y, CurrLocation.Z);
				Result.Push(NewPathNode);
				CurrNodePtr = CurrNodePtr->PrevNode;
			}
			Algo::Reverse(Result);
		}

		for (const FAStarNode* NodePtr : NodesToFree) {
			delete NodePtr;
		}

		return Result;
	}
};
