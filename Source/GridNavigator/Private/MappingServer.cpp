#include "MappingServer.h"

#include <sstream>

#include "GridNavigatorConfig.h"
#include "Engine/World.h"

bool FloorTrace(const float I, const float J, const float MaxZ, const float MinZ, FHitResult& HitResult, const UWorld& World)
{
	const FIntVector2 GridIndex(I, J);
	const FVector2f WorldCoordXY = FMappingServer::GridIndexToWorld(GridIndex);
	const FVector WorldLocationTraceStart(WorldCoordXY.X, WorldCoordXY.Y, MaxZ * 25.0);
	const FVector WorldLocationTraceEnd  (WorldCoordXY.X, WorldCoordXY.Y, MinZ * 25.0);
	
	const FCollisionQueryParams TraceParams(FName(TEXT("FMappingServerTraceParams")));
	
	return World.LineTraceSingleByObjectType(HitResult, WorldLocationTraceStart, WorldLocationTraceEnd, ECC_WorldStatic);
}

bool SubGridFloorTrace(const int I, const int J, const float MaxZ, const float MinZ, const FIntVector2 Direction, const float Alpha, FHitResult& HitResult, const UWorld& World)
{
	const FVector2f GridIndex(I, J);
	const FVector2f WorldCoordXY = FMappingServer::SubGridIndexToWorld(GridIndex, Direction, Alpha);
	const FVector WorldLocationTraceStart(WorldCoordXY.X, WorldCoordXY.Y, MaxZ * 25.0);
	const FVector WorldLocationTraceEnd  (WorldCoordXY.X, WorldCoordXY.Y, MinZ * 25.0);
	
	const FCollisionQueryParams TraceParams(FName(TEXT("FMappingServerTraceParams")));
	
	return World.LineTraceSingleByObjectType(HitResult, WorldLocationTraceStart, WorldLocationTraceEnd, ECC_WorldStatic);
}

bool IsRoughlyEqual(const float Lhs, const float Rhs, const float Tolerance)
{
	return FMath::Abs(Lhs - Rhs) < Tolerance;
}

void FMappingServer::RemapFromBound(const UWorld& World, const FBox& Bound)
{
	Map.Clear();

	PopulateMap(World, Bound);
}

TArray<FVector> FMappingServer::FindPath(const FVector& From, const FVector& To)
{
	FIntVector3 FromGridCoord = WorldToGridIndex(From);
	FIntVector3 ToGridCoord   = WorldToGridIndex(To);
	
	const TArray<FVector> IndexPoints = Map.FindPath(FromGridCoord, ToGridCoord);

	// map (I,J,Z) triplets to (X,Y,Z) triplets
	TArray<FVector> WorldPoints;
	for (const FVector& PointIndex : IndexPoints) {
		FVector PointWorld = GridIndexToWorld(PointIndex);
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

FBoxSphereBounds FMappingServer::GetBounds() const
{
	FVector MinBound(TNumericLimits<float>::Max());
	FVector MaxBound(TNumericLimits<float>::Min());

	// iterate over all nodes to determine max and min bounding points
	TArray<FMapAdjacencyList::FNode> Nodes;
	Map.Nodes.GenerateValueArray(Nodes);
	for (const auto& Node : Nodes) {
		if (Node.X < MinBound.X) MinBound.X = Node.X;
		if (Node.X > MaxBound.X) MaxBound.X = Node.X;
		if (Node.Y < MinBound.Y) MinBound.Y = Node.Y;
		if (Node.Y > MaxBound.Y) MaxBound.Y = Node.Y;
		if (Node.Layer < MinBound.Z) MinBound.Z = Node.Layer;
		if (Node.Layer > MaxBound.Z) MaxBound.Z = Node.Layer;
	}

	const FVector BoundsOrigin = (MinBound + MaxBound) / 2.f;
	const FVector BoundsExtent = (MaxBound - BoundsOrigin);

	const FVector MapSizeVec(100.0, 100.0, 25.0);
	const FVector BoundsOriginScaled = BoundsOrigin * MapSizeVec;
	const FVector BoundsExtentScaled = BoundsExtent * MapSizeVec;
	const float SphereRadius = BoundsExtentScaled.Size();

	return FBoxSphereBounds(BoundsOriginScaled, BoundsExtentScaled, SphereRadius);
}

TArray<FMapAdjacencyList::FNode> FMappingServer::GetMapNodeList()
{
	return Map.GetNodeList();
}

TArray<FMapAdjacencyList::FEdge> FMappingServer::GetMapEdgeList()
{
	return Map.GetEdgeList();
}

std::optional<std::reference_wrapper<const FMapAdjacencyList::FNode>> FMappingServer::GetNode(const FMapAdjacencyList::FNode::ID ID)
{
	if (!Map.Nodes.Contains(ID)) {
		return std::nullopt;
	}
	return Map.Nodes[ID];
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

void FMappingServer::Serialize(FArchive& Ar)
{
	Ar << Map.Nodes;
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
		FMath::RoundToInt(WorldCoord.X / static_cast<float>(GridNavigatorConfig::GridSizeX)),
		FMath::RoundToInt(WorldCoord.Y / static_cast<float>(GridNavigatorConfig::GridSizeY))
	);
}

FIntVector3 FMappingServer::WorldToGridIndex(const FVector& WorldCoord)
{
	return FIntVector3(
		FMath::RoundToInt(WorldCoord.X / static_cast<float>(GridNavigatorConfig::GridSizeX)),
		FMath::RoundToInt(WorldCoord.Y / static_cast<float>(GridNavigatorConfig::GridSizeY)),
		FMath::RoundToInt(WorldCoord.Z / static_cast<float>(GridNavigatorConfig::GridSizeLayer))
	);
}

FVector2f FMappingServer::GridIndexToWorld(const FIntVector2& IndexCoord)
{
	return FVector2f(
		static_cast<float>(IndexCoord.X) * static_cast<float>(GridNavigatorConfig::GridSizeX),
		static_cast<float>(IndexCoord.Y) * static_cast<float>(GridNavigatorConfig::GridSizeY)
	);
}

FVector FMappingServer::GridIndexToWorld(const FIntVector3& IndexCoord)
{
	return FVector(
		static_cast<float>(IndexCoord.X) * static_cast<float>(GridNavigatorConfig::GridSizeX),
		static_cast<float>(IndexCoord.Y) * static_cast<float>(GridNavigatorConfig::GridSizeY),
		static_cast<float>(IndexCoord.Z) * static_cast<float>(GridNavigatorConfig::GridSizeLayer)
	);
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

	const int MinX = FMath::RoundToInt(BoundingBox.Min.X / 100.0);
	const int MinY = FMath::RoundToInt(BoundingBox.Min.Y / 100.0);
	const int MinZ = FMath::RoundToInt(BoundingBox.Min.Z / 25.0);
	const int MaxX = FMath::RoundToInt(BoundingBox.Max.X / 100.0);
	const int MaxY = FMath::RoundToInt(BoundingBox.Max.Y / 100.0);
	const int MaxZ = FMath::RoundToInt(BoundingBox.Max.Z / 25.0);

	for (int i = MinX; i <= MaxX; ++i) {
		for (int j = MinY; j <= MaxY; ++j) {
			FHitResult HitResult(ForceInit);
			const bool NodeIJExists = FloorTrace(i, j, MaxZ, MinZ, HitResult, World);
			
			if (!NodeIJExists) {
				continue;
			}

			FHitResult CeilHitResult(ForceInit);
			const int Layer = FMath::RoundToInt(HitResult.Location.Z / 25.0);
			const bool bShortRoofOverHitPoint = FloorTrace(i, j, Layer, Layer + GridNavigatorConfig::MinEmptyLayersForValidNode, CeilHitResult, World);

			// end loop if there's nowhere to stand; also, ignore walls that might be directly
			// on top of the node (ie. have a near-zero Z component for normal vector)
			if (bShortRoofOverHitPoint && CeilHitResult.Normal.Z > 1.0) {
				continue;
			}
			
			Map.AddNode(i, j, Layer, HitResult.Location.Z);

			for (const auto& [NeighborI, NeighborJ] : Neighbors) {
				FHitResult NeighborHitResult(ForceInit);
				FIntVector3 NeighborIndex(i + NeighborI, j + NeighborJ, 0);
				const bool NeighborNodeExists = FloorTrace(NeighborIndex.X, NeighborIndex.Y, MaxZ, MinZ, NeighborHitResult, World);
			
				if (!NeighborNodeExists) {
					continue;
				}

				// if neighbor is outside bounding box, quit early
				if (!BoundingBox.IsInside(NeighborHitResult.Location)) {
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
					bool NodeSideSubGridFloorExists = SubGridFloorTrace(i, j, MaxZ, MinZ, NeighborDir, 0.2, NodeSideSubGridHitResult, World);

					FHitResult NeighborSideSubGridHitResult;
					bool NeighborSideSubGridFloorExists = SubGridFloorTrace(i, j, MaxZ, MinZ, NeighborDir, 0.8, NeighborSideSubGridHitResult, World);

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
					const bool DidMidpointTraceHit = FloorTrace(Midpoint.X, Midpoint.Y, MaxZ, MinZ, MidpointHitResult, World);

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

				const int FromLayer = FMath::RoundToInt(NodeHeight / 25.0);
				const int ToLayer   = FMath::RoundToInt(NeighborHeight /  25.0);
			
				Map.CreateEdge(
					i,             j,             FromLayer, NodeHeight,
					i + NeighborI, j + NeighborJ, ToLayer,   NeighborHeight,
					EdgeType
				);
			}
		}
	}
}
