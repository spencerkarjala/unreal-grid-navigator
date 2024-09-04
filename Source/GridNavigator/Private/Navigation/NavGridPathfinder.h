#pragma once

#include "MapData/NavGridAdjacencyList.h"

/**
 * @class FNavGridPathfinder
 * @brief Performs pathfinding on a navigation grid.
 **/
class FNavGridPathfinder
{
public:
	/**
	 * Finds a path between two nodes in the grid.
	 * 
	 * @param WorldRef A reference to the UWorld context object.
	 * @param Grid The navigation grid to search through.
	 * @param First The world position for the start of pathfinding.
	 * @param Final The world position for the end of pathfinding.
	 * @return A list of nodes representing the path from the First point to the Final point.
	 */
	static TArray<FVector> FindPath(const UWorld& WorldRef, const FNavGridAdjacencyList& Grid, const FVector& First, const FVector& Final);
};
