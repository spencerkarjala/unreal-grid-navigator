#include "NavGridBuildTask.h"

#include "GridNavigatorConfig.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridBuildTask, Log, All);

FNavGridBuildTask::FNavGridBuildTask(UWorld* World, ANavigationGridData* Data) : WorldRef(World), DataRef(Data) {}

TStatId FNavGridBuildTask::GetStatId() const 
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(GNDataBuildTask, STATGROUP_ThreadPoolAsyncTasks);
}

bool FNavGridBuildTask::CanAbandon() const 
{
	return false;
}

void FNavGridBuildTask::DoWork() const
{
	if (!WorldRef) {
		UE_LOG(LogNavGridBuildTask, Error, TEXT("Tried to rebuild navigation grid data without a valid world reference"));
		return;
	}
	if (!DataRef) {
		UE_LOG(LogNavGridBuildTask, Error, TEXT("Tried to rebuild navigation grid data without a valid data reference"));
		return;
	}

	const auto LevelData = DataRef->GetLevelData();
	for (auto& [ID, Block] : LevelData->Blocks) {
		PopulateBlock(*WorldRef, Block);
	}

	auto Result = OnCompleted.ExecuteIfBound();
}

FVector2f SubGridIndexToWorld(const FVector2f& IndexCoord, FIntVector2 Direction, const float Alpha)
{
	// make sure direction vectors only include {-1,0,1}
	FVector2f UnitDirection = FVector2f(Direction.X, Direction.Y);
	UnitDirection.Normalize();
	UnitDirection = FVector2f(FMath::RoundToFloat(UnitDirection.X), FMath::RoundToFloat(UnitDirection.Y));

	FVector2f WorldCoords(IndexCoord.X * GridNavigatorConfig::ASSUMED_GRID_SPACING, IndexCoord.Y * GridNavigatorConfig::ASSUMED_GRID_SPACING);
	
	return WorldCoords + UnitDirection * Alpha * GridNavigatorConfig::ASSUMED_GRID_SPACING;
}

bool FloorTrace(const float I, const float J, const float MaxZ, const float MinZ, FHitResult& HitResult, const UWorld& World)
{
	const FVector2f GridIndex(I, J);
	const FVector2f WorldCoordXY = GridNavigatorConfig::GridIndexToWorld(GridIndex);
	const FVector WorldLocationTraceStart(WorldCoordXY.X, WorldCoordXY.Y, MaxZ * 25.0);
	const FVector WorldLocationTraceEnd  (WorldCoordXY.X, WorldCoordXY.Y, MinZ * 25.0);
	
	const FCollisionQueryParams TraceParams(FName(TEXT("FNavGridBuildTaskTraceParams")));
	
	return World.LineTraceSingleByObjectType(HitResult, WorldLocationTraceStart, WorldLocationTraceEnd, ECC_WorldStatic);
}

bool SubGridFloorTrace(const int I, const int J, const float MaxZ, const float MinZ, const FIntVector2 Direction, const float Alpha, FHitResult& HitResult, const UWorld& World)
{
	const FVector2f GridIndex(I, J);
	const FVector2f WorldCoordXY = SubGridIndexToWorld(GridIndex, Direction, Alpha);
	const FVector WorldLocationTraceStart(WorldCoordXY.X, WorldCoordXY.Y, MaxZ * 25.0);
	const FVector WorldLocationTraceEnd  (WorldCoordXY.X, WorldCoordXY.Y, MinZ * 25.0);
	
	const FCollisionQueryParams TraceParams(FName(TEXT("FNavGridBuildTaskTraceParams")));
	
	return World.LineTraceSingleByObjectType(HitResult, WorldLocationTraceStart, WorldLocationTraceEnd, ECC_WorldStatic);
}

bool IsRoughlyEqual(const float Lhs, const float Rhs, const float Tolerance)
{
	return FMath::Abs(Lhs - Rhs) < Tolerance;
}

void FNavGridBuildTask::PopulateBlock(const UWorld& World, FNavGridBlock& Block)
{
	const TArray<TPair<int, int>> Neighbors = {{1, 0}, {1, 1},{0, 1},{-1, 1},{-1, 0},{-1, -1},{0, -1},{1, -1} };
	
	const auto& BoundingBox = Block.Bounds;
	auto& Map = Block.Data;

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
			if (bShortRoofOverHitPoint && FMath::Abs(CeilHitResult.Normal.Z) > 0.1) {
				continue;
			}
			
			Map.AddNode(i, j, Layer, HitResult.Location.Z);
			UE_LOG(LogNavGridBuildTask, Verbose, TEXT("Added new node to nav grid at indices (%d, %d, %d)"), i, j, Layer);

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
				
				NavGrid::EMapEdgeType EdgeType = NavGrid::EMapEdgeType::None;

				if (HeightDelta <= 1.0) {
					EdgeType = NavGrid::EMapEdgeType::Direct;
				}
				else if (HeightDelta <= 51.0 && !IsDiagonal) {
					EdgeType = NavGrid::EMapEdgeType::Slope;
				}
				else if (NodeHeight != NeighborHeight) {
					EdgeType = NavGrid::EMapEdgeType::Cliff;
				}

				// avoids stepping up/down on the edges of slopes from flat ground
				if (EdgeType == NavGrid::EMapEdgeType::Slope) {
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

						if (SlopesAreEqual && SlopesAreFlat && NodeHeight != NeighborHeight) {
							EdgeType = NavGrid::EMapEdgeType::Cliff;
						}
					}
				}

				// include cases for sloping upwards + downwards so later pathfinding can figure out
				// what height to put sub-grid points at (ie. tops/bottoms of slopes for path previews)
				if (EdgeType == NavGrid::EMapEdgeType::Slope) {
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
						if      (NodeIsFlatSide     && NodeIsLowerSide)     EdgeType = NavGrid::EMapEdgeType::SlopeBottom;
						else if (NodeIsFlatSide     && NeighborIsLowerSide) EdgeType = NavGrid::EMapEdgeType::SlopeTop;
						else if (NeighborIsFlatSide && NodeIsLowerSide)     EdgeType = NavGrid::EMapEdgeType::SlopeTop;
						else if (NeighborIsFlatSide && NeighborIsLowerSide) EdgeType = NavGrid::EMapEdgeType::SlopeBottom;
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
