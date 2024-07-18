#include "MappingServer.h"

#include <sstream>

#include "Engine/World.h"

bool FloorTrace(const float I, const float J, FHitResult& HitResult, const UWorld& World)
{
	constexpr float TraceHeight = 1000.0;
	
	const FVector2f GridIndex(I, J);
	const FVector2f WorldCoordXY = FMappingServer::GridIndexToWorld(GridIndex);
	const FVector WorldLocationTraceStart(WorldCoordXY.X, WorldCoordXY.Y, TraceHeight / 2.0);
	const FVector WorldLocationTraceEnd  (WorldCoordXY.X, WorldCoordXY.Y, WorldLocationTraceStart.Z - TraceHeight);
	
	const FCollisionQueryParams TraceParams(FName(TEXT("FMappingServerTraceParams")));
	
	return World.LineTraceSingleByObjectType(HitResult, WorldLocationTraceStart, WorldLocationTraceEnd, ECC_WorldStatic);
}

bool SubGridFloorTrace(const int I, const int J, const FIntVector2 Direction, const float Alpha, FHitResult& HitResult, const UWorld& World)
{
	constexpr float TraceHeight = 1000.0;

	const FVector2f GridIndex(I, J);
	const FVector2f WorldCoordXY = FMappingServer::SubGridIndexToWorld(GridIndex, Direction, Alpha);
	const FVector WorldLocationTraceStart(WorldCoordXY.X, WorldCoordXY.Y, TraceHeight / 2.0);
	const FVector WorldLocationTraceEnd  (WorldCoordXY.X, WorldCoordXY.Y, WorldLocationTraceStart.Z - TraceHeight);
	
	const FCollisionQueryParams TraceParams(FName(TEXT("FMappingServerTraceParams")));
	
	return World.LineTraceSingleByObjectType(HitResult, WorldLocationTraceStart, WorldLocationTraceEnd, ECC_WorldStatic);
}

bool IsRoughlyEqual(const float Lhs, const float Rhs, const float Tolerance)
{
	return FMath::Abs(Lhs - Rhs) < Tolerance;
}

void FMappingServer::RemapFromWorld(const UWorld& World)
{
	constexpr int HalfMapSize = ASSUMED_MAX_MAP_SIZE / 2;
	constexpr int Layer = 0;

	Map.Clear();

	const FVector LowerBound(-HalfMapSize, -HalfMapSize, Layer);
	const FVector UpperBound(HalfMapSize, HalfMapSize, Layer);

	PopulateMap(World, FBox(LowerBound, UpperBound));
}


TArray<FVector> FMappingServer::FindPath(const FVector& From, const FVector& To)
{
	FVector2f FromWorldXY(From.X, From.Y);
	FVector2f ToWorldXY(To.X, To.Y);

	FIntVector2 FromGridCoord = WorldToGridIndex(FromWorldXY);
	FIntVector2 ToGridCoord   = WorldToGridIndex(ToWorldXY);
	
	const TArray<FVector> IndexPoints = Map.FindPath(FromGridCoord, ToGridCoord);

	// map (I,J,Z) triplets to (X,Y,Z) triplets
	TArray<FVector> WorldPoints;
	for (const FVector& Point : IndexPoints) {
		FVector2f PointIndex(Point.X, Point.Y);
		FVector2f PointWorldXY = GridIndexToWorld(PointIndex);
		FVector PointWorld(PointWorldXY.X, PointWorldXY.Y, Point.Z);
		WorldPoints.Add(PointWorld);
	}

	return WorldPoints;
}

TPair<TArray<FVector>, TArray<FVector>> FMappingServer::FindPath(const FVector& From, const FVector& To, const float DistanceBudget)
{
	TArray<FVector> Path = FindPath(From, To);
	TArray<FVector> NavigablePoints;
	TArray<FVector> FilteredPoints;
	
	float TotalDistance = 0.f;
	const float AvailableMoveDistance = DistanceBudget;

	if (Path.Num() > 0) {
		NavigablePoints.Add(Path[0]);
	}
	
	for (int i = 1; i < Path.Num(); ++i) {
		const auto P0 = Path[i-1];
		const auto P1 = Path[i];
		const float DistanceP0P1 = (P1 - P0).Size();

		// case 1: all points on the line [P0,P1) are within the distance budget
		if (TotalDistance + DistanceP0P1 <= AvailableMoveDistance) {
			NavigablePoints.Add(P1);
		}
		// case 2: all points on the line [P0,P1) are outside the distance budget
		else if (AvailableMoveDistance <= TotalDistance) {
			FilteredPoints.Add(P1);
		}
		// case 3: at some point on the line [P0,P1), move budget runs out
		else if (TotalDistance < AvailableMoveDistance) {
			const float RemainingDistanceBudget = AvailableMoveDistance - TotalDistance;
			ensure(0.f < RemainingDistanceBudget && RemainingDistanceBudget < DistanceP0P1);
			// lerp to find crossover from in-budget to out-of-budget
			const auto P2 = P0 + (P1 - P0) * (RemainingDistanceBudget / DistanceP0P1);

			NavigablePoints.Add(P2);
			FilteredPoints.Add(P1);
		}

		TotalDistance += DistanceP0P1;
	}

	return { NavigablePoints, FilteredPoints };
}

FVector FMappingServer::RoundToGrid(const FVector& Value)
{
	return FVector(
		round(Value.X / 100.0) * 100.00,
		round(Value.Y / 100.0) * 100.00,
		round(Value.Z / 25.0) * 25.0
	);
}

FVector FMappingServer::TruncToGrid(const FVector& Value)
{
	return FVector(
		floor(Value.X / 100.0) * 100.0,
		floor(Value.Y / 100.0) * 100.0,
		floor(Value.Z / 25.0) * 25.0
	);
}

FString FMappingServer::Stringify()
{
	return this->Map.Stringify();
}

void FMappingServer::DrawDebug(const UWorld& World)
{
	Map.DrawDebug(World);
}

FIntVector2 FMappingServer::WorldToGridIndex(const FVector2f& WorldCoord)
{
	return FIntVector2(
		FMath::RoundToInt(WorldCoord.X / static_cast<float>(ASSUMED_GRID_SPACING)),
		FMath::RoundToInt(WorldCoord.Y / static_cast<float>(ASSUMED_GRID_SPACING))
	);
}

FVector2f FMappingServer::GridIndexToWorld(const FVector2f& IndexCoord)
{
	return IndexCoord * ASSUMED_GRID_SPACING;
}

FVector2f FMappingServer::SubGridIndexToWorld(const FVector2f& IndexCoord, FIntVector2 Direction, const float Alpha)
{
	// make sure direction vectors only include {-1,0,1}
	FVector2f UnitDirection = FVector2f(Direction.X, Direction.Y);
	UnitDirection.Normalize();
	UnitDirection = FVector2f(FMath::RoundToFloat(UnitDirection.X), FMath::RoundToFloat(UnitDirection.Y));

	FVector2f WorldCoords(IndexCoord.X * ASSUMED_GRID_SPACING, IndexCoord.Y * ASSUMED_GRID_SPACING);
	
	return WorldCoords + UnitDirection * Alpha * ASSUMED_GRID_SPACING;
}

void FMappingServer::PopulateMap(const UWorld& World, const FBox& BoundingBox)
{
	const TArray<TPair<int, int>> Neighbors = {{1, 0}, {1, 1},{0, 1},{-1, 1},{-1, 0},{-1, -1},{0, -1},{1, -1} };
	const auto& MinP = BoundingBox.Min;
	const auto& MaxP = BoundingBox.Max;

	for (int i = MinP.X; i <= MaxP.X; ++i) {
		for (int j = MinP.Y; j <= MaxP.Y; ++j) {
			FHitResult HitResult(ForceInit);
			const bool NodeIJExists = FloorTrace(i, j, HitResult, World);
			
			if (!NodeIJExists) {
				continue;
			}

			// const int Layer = round(HitResult.Location.Z / 25.0);
			constexpr int Layer = 0;
			
			Map.AddNode(i, j, Layer, HitResult.Location.Z);

			for (const auto& [NeighborI, NeighborJ] : Neighbors) {
				FHitResult NeighborHitResult(ForceInit);
				const bool NeighborNodeExists = FloorTrace(i + NeighborI, j + NeighborJ, NeighborHitResult, World);
			
				if (!NeighborNodeExists) {
					continue;
				}

				const float NodeHeight = HitResult.Location.Z;
				const float NeighborHeight = NeighborHitResult.Location.Z;

				FVector ObstrTraceStart = HitResult.Location;
				FVector ObstrTraceEnd   = NeighborHitResult.Location;

				if (NodeHeight < NeighborHeight) {
					ObstrTraceStart.Z = NeighborHeight;
				}
				else if (NeighborHeight < NodeHeight) {
					ObstrTraceEnd.Z = NodeHeight;
				}

				// add a little bit of height to avoid floor collisions
				ObstrTraceStart.Z += 5.0;
				ObstrTraceEnd.Z   += 5.0;

				FHitResult WallHitResult;
				FCollisionObjectQueryParams ObjectsToTrace = ECC_WorldStatic;
				
				const bool IsObstructedLow = World.LineTraceSingleByObjectType(WallHitResult, ObstrTraceStart, ObstrTraceEnd, ObjectsToTrace);
				if (IsObstructedLow) {
					continue;
				}

				// do a second pass higher up; handles the case where there might be a gap by
				// character's feet but something to collide with near their head
				ObstrTraceStart.Z += 100.0;
				ObstrTraceEnd.Z   += 100.0;

				const bool IsObstructedHigh = World.LineTraceSingleByObjectType(WallHitResult, ObstrTraceStart, ObstrTraceEnd, ObjectsToTrace);
				if (IsObstructedHigh) {
					continue;
				}

				const float HeightDelta = FMath::Abs(NodeHeight - NeighborHeight);
				const bool IsDiagonal = (NeighborI != 0) && (NeighborJ != 0); 
				
				FMapAdjacencyList::EMapEdgeType EdgeType = FMapAdjacencyList::EMapEdgeType::None;

				if (HeightDelta <= 1.0) {
					EdgeType = FMapAdjacencyList::EMapEdgeType::Direct;
				}
				else if (HeightDelta <= 51.0 && !IsDiagonal) {
					EdgeType = FMapAdjacencyList::EMapEdgeType::Slope;
				}
				else if (NodeHeight < NeighborHeight) {
					EdgeType = FMapAdjacencyList::EMapEdgeType::CliffUp;
				}
				else if (NodeHeight > NeighborHeight) {
					EdgeType = FMapAdjacencyList::EMapEdgeType::CliffDown;
				}

				// avoids stepping up/down on the edges of slopes from flat ground
				if (EdgeType == FMapAdjacencyList::EMapEdgeType::Slope) {
					const FIntVector2 NeighborDir(NeighborI, NeighborJ);
					
					FHitResult NodeSideSubGridHitResult;
					bool NodeSideSubGridFloorExists = SubGridFloorTrace(i, j, NeighborDir, 0.2, NodeSideSubGridHitResult, World);

					FHitResult NeighborSideSubGridHitResult;
					bool NeighborSideSubGridFloorExists = SubGridFloorTrace(i, j, NeighborDir, 0.8, NeighborSideSubGridHitResult, World);

					if (NodeSideSubGridFloorExists && NeighborSideSubGridFloorExists) {
						const FVector& NodeSideSubGridLocation     = NodeSideSubGridHitResult.Location;
						const FVector& NeighborSideSubGridLocation = NeighborSideSubGridHitResult.Location;

						const float NodeSlopeZ     = FMath::Abs(NodeSideSubGridLocation.Z - NodeHeight);
						const float NeighborSlopeZ = FMath::Abs(NeighborSideSubGridLocation.Z - NeighborHeight);

						const bool SlopesAreEqual = FMath::Abs(NodeSlopeZ - NeighborSlopeZ) < 1.0;
						const bool SlopesAreFlat = NodeSlopeZ + NeighborSlopeZ < 5.0;

						if (SlopesAreEqual && SlopesAreFlat && NodeHeight < NeighborHeight) {
							EdgeType = FMapAdjacencyList::EMapEdgeType::CliffUp;
						}
						else if (SlopesAreEqual && SlopesAreFlat && NodeHeight > NeighborHeight) {
							EdgeType = FMapAdjacencyList::EMapEdgeType::CliffDown;
						}
					}
				}

				// include cases for sloping upwards + downwards so later pathfinding can figure out
				// what height to put sub-grid points at (ie. tops/bottoms of slopes for path previews)
				if (EdgeType == FMapAdjacencyList::EMapEdgeType::Slope) {
					const FVector2f PointA(i, j);
					const FVector2f PointB(i + NeighborI, j + NeighborJ);
					const FVector2f Midpoint = (PointA + PointB) / 2.0;

					FHitResult MidpointHitResult;
					const bool DidMidpointTraceHit = FloorTrace(Midpoint.X, Midpoint.Y, MidpointHitResult, World);

					// tracing between two valid navmesh points should never fail (ie. no gaps)
					check(DidMidpointTraceHit);

					const float MidpointHeight = MidpointHitResult.Location.Z;
					const float AverageHeight = (NodeHeight + NeighborHeight) / 2.f;

					const bool IsMiddleOfSlope = IsRoughlyEqual(MidpointHeight, AverageHeight, 1.0);

					// if current edge is starting or ending slope, then update its edge type to match that
					if (!IsMiddleOfSlope) {
						const bool NodeIsFlatSide     = IsRoughlyEqual(NodeHeight, MidpointHeight, 1.0);
						const bool NeighborIsFlatSide = !NodeIsFlatSide;
						const bool NodeIsLowerSide     = NodeHeight < NeighborHeight;
						const bool NeighborIsLowerSide = !NodeIsLowerSide;

						// truth table time
						// should probably refactor this to just use an array lookup
						if      (NodeIsFlatSide     && NodeIsLowerSide)     EdgeType = FMapAdjacencyList::EMapEdgeType::SlopeBottom;
						else if (NodeIsFlatSide     && NeighborIsLowerSide) EdgeType = FMapAdjacencyList::EMapEdgeType::SlopeTop;
						else if (NeighborIsFlatSide && NodeIsLowerSide)     EdgeType = FMapAdjacencyList::EMapEdgeType::SlopeTop;
						else if (NeighborIsFlatSide && NeighborIsLowerSide) EdgeType = FMapAdjacencyList::EMapEdgeType::SlopeBottom;
					}
				}
			
				Map.CreateEdge(
					i,             j,             Layer, NodeHeight,
					i + NeighborI, j + NeighborJ, Layer, NeighborHeight,
					EdgeType
				);
			}
		}
	}
}

