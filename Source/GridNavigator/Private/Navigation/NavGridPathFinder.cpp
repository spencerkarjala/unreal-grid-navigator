#include "NavGridPathFinder.h"

#include "AStarNavigator.h"
#include "GridNavigatorConfig.h"
#include "MapData/NavGridAdjacencyList.h"

namespace UE::Math
{
	inline double Distance(const FInt64Vector3& Lhs, const FInt64Vector3& Rhs)
	{
		const FInt64Vector3 Diff = Rhs - Lhs;
		const double XSquared = static_cast<double>(Diff.X) * static_cast<double>(Diff.X);
		const double YSquared = static_cast<double>(Diff.Y) * static_cast<double>(Diff.Y);
		const double ZSquared = static_cast<double>(Diff.Z) * static_cast<double>(Diff.Z);
		return FMath::Sqrt(XSquared + YSquared + ZSquared);
	}
}

TArray<FVector> FNavGridPathfinder::FindPath(const UWorld& WorldRef, const FNavGridAdjacencyList& Grid, const FVector& First, const FVector& Final)
{
	FInt64Vector3 FirstIndex = GridNavigatorConfig::WorldToGridIndex(First);
	FInt64Vector3 FinalIndex = GridNavigatorConfig::WorldToGridIndex(Final);
	
    // ensure start and end nodes exist in the grid
    if (!Grid.HasNode(FirstIndex.X, FirstIndex.Y, FirstIndex.Z) || !Grid.HasNode(FinalIndex.X, FinalIndex.Y, FinalIndex.Z)) {
        return {};
    }

	// Call the A* algorithm
	TAStarNavigator<FNavGridAdjacencyList, FInt64Vector3> Navigator;
	Navigator.Heuristic = [](const FInt64Vector3& Lhs, const FInt64Vector3& Rhs) -> double
	{
		const double YComponent = static_cast<double>(Rhs.Y - Lhs.Y);
		const double XComponent = static_cast<double>(Rhs.X - Lhs.X);
		return FMath::Sqrt(XComponent*XComponent + YComponent*YComponent);
	};

    TArray<FInt64Vector> PathNodes = Navigator.Navigate(Grid, FirstIndex, FinalIndex);
    if (PathNodes.Num() == 0) {
        return {};
    }

    // Perform path smoothing and additional geometry processing as needed
    TArray<FVector> UnfilteredPath;
	FVector PointA = GridNavigatorConfig::GridIndexToWorld(PathNodes[0]);
    for (int i = 1; i < PathNodes.Num(); ++i) {
        UnfilteredPath.Add(PointA);
        const FVector PointB = GridNavigatorConfig::GridIndexToWorld(PathNodes[i]);

        if (PointA.Z != PointB.Z) {
        	FVector TraceUpper = (PointA + PointB) / 2.0;
        	FVector TraceLower = TraceUpper;
        	TraceUpper.Z = FMath::Max(PointA.Z, PointB.Z) + 1.0;
        	TraceLower.Z = FMath::Min(PointA.Z, PointB.Z) - 1.0;

        	FHitResult TraceResult;
        	FCollisionObjectQueryParams ObjectsToTrace(ECC_WorldStatic);
        	WorldRef.LineTraceSingleByObjectType(TraceResult, TraceUpper, TraceLower, ObjectsToTrace);

        	// trace should never miss (no gaps between nodes in a path)
        	check(TraceResult.IsValidBlockingHit());

        	UnfilteredPath.Add(TraceResult.Location);
        }

        PointA = PointB;
    }

    UnfilteredPath.Add(PointA);

    // Optional: Filter collinear points from the path
    TArray<FVector> Path;
    FVector LineStart = UnfilteredPath[0];
    Path.Add(LineStart);

    for (int i = 2; i < UnfilteredPath.Num(); ++i) {
        const FVector& LineEnd = UnfilteredPath[i];
        const FVector& TestPoint = UnfilteredPath[i - 1];

        UE::Geometry::FLine3d StartEndLine = UE::Geometry::FLine3d::FromPoints(LineStart, LineEnd);
        float DistanceToLine = StartEndLine.DistanceSquared(TestPoint);

        if (DistanceToLine >= UE_KINDA_SMALL_NUMBER) {
            Path.Add(TestPoint);
            LineStart = TestPoint;
        }
    }

    Path.Add(UnfilteredPath.Last());
    return Path;
}
