#include "GNRecastNavMesh.h"

#include "NavigationData.h"
#include "NavigationSystem.h"

#include <functional>

#include "NavGridRenderingComponent.h"
#include "MappingServer.h"
#include "NavMesh/PImplRecastNavMesh.h"
#include "GNNavDataGenerator.h"

DECLARE_LOG_CATEGORY_CLASS(LogGNRecastNavMesh, Log, All);

AGNRecastNavMesh::AGNRecastNavMesh(const FObjectInitializer& ObjectInitializer) : ARecastNavMesh(ObjectInitializer)
{
	FindPathImplementation = this->FindPath;
}

void AGNRecastNavMesh::OnNavigationBoundsChanged()
{
	HandleRebuildNavigation();
}

UPrimitiveComponent* AGNRecastNavMesh::ConstructRenderingComponent()
{
	DebugRenderingComponent = NewObject<UNavGridRenderingComponent>(this, TEXT("NavGridRenderingComponent"), RF_Transient);
	if (!DebugRenderingComponent) {
		return nullptr;
	}
	return DebugRenderingComponent.Get();
}

void AGNRecastNavMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	FMappingServer::GetInstance().Serialize(Ar);
}

void AGNRecastNavMesh::ConditionalConstructGenerator()
{
	if (NavDataGenerator.IsValid()) {
		NavDataGenerator->CancelBuild();
		NavDataGenerator.Reset();
	}

	UWorld* World = GetWorld();
	check(World);
	const bool bRequiresGenerator = SupportsRuntimeGeneration() || !World->IsGameWorld();
	if (!bRequiresGenerator) {
		return;
	}
	
	FGNNavDataGenerator* Generator = new FGNNavDataGenerator();
	if (Generator == nullptr) {
		UE_LOG(LogGNRecastNavMesh, Error, TEXT("Failed to instantiate new GNNavDataGenerator"));
		return;
	}
	NavDataGenerator = MakeShareable(static_cast<FNavDataGenerator*>(Generator));
}

void AGNRecastNavMesh::RebuildNavigation() const
{
	HandleRebuildNavigation();
}

void AGNRecastNavMesh::HandleRebuildNavigation() const
{
	const auto* World = GetWorld();
	if (!IsValid(World)) {
		return;
	}
	
	const auto& NavigableBounds = GetNavigableBounds();
	for (const auto& Bound : NavigableBounds) {
		const auto MinIndex = FMappingServer::RoundToGrid(Bound.Min);
		const auto MaxIndex = FMappingServer::RoundToGrid(Bound.Max);

		FMappingServer::GetInstance().RemapFromBound(*World, FBox(MinIndex, MaxIndex));

		break;
	}
	if (DebugRenderingComponent) {
		DebugRenderingComponent->MarkRenderStateDirty();
	}
}

struct Point {
	int x, y;

	Point(int _x, int _y)
	{
		this->x = _x;
		this->y = _y;
	}

	Point(float _x, float _y)
	{
		this->x = round(_x);
		this->y = round(_y);
	}

	Point(double _x, double _y)
	{
		this->x = round(_x);
		this->y = round(_y);
	}

	explicit Point(const FVector& rhs)
	{
		this->x = round(rhs.X);
		this->y = round(rhs.Y);
	}
	
	const Point& operator=(const FVector& rhs) const
	{
		return Point(rhs);
	}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    // Euclidean distance for heuristic
    double DistanceTo(const Point& other) const {
        return std::hypot(x - other.x, y - other.y);
    }
};

namespace std {
    template <>
    struct hash<Point> {
        size_t operator()(const Point& p) const {
            return hash<int>()(p.x) ^ hash<int>()(p.y);
        }
    };
}

FPathFindingResult AGNRecastNavMesh::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	DECLARE_CYCLE_STAT(TEXT("Grid Pathfinding"), STAT_Navigation_GridPathfinding, STATGROUP_Navigation);
	SCOPE_CYCLE_COUNTER(STAT_Navigation_GridPathfinding);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	FPathFindingResult Result(ENavigationQueryResult::Error);

	UE_LOG(LogGNRecastNavMesh, Log, TEXT("Got cost limit of: '%0.2f'"), Query.CostLimit);

	const auto* Self = Cast<const ARecastNavMesh>(Query.NavData.Get());
	const auto* NavFilter = Query.QueryFilter.Get();

	if (!Self) {
		UE_LOG(LogGNRecastNavMesh, Error, TEXT("Failed to retrieve reference to RecastNavMesh in FindPath"));
		return Result;
	}
	if (!NavFilter) {
		UE_LOG(LogGNRecastNavMesh, Error, TEXT("Failed to retrieve reference to query filter in FindPath"));
		return Result;
	}
	
	auto* NavPath = Query.PathInstanceToFill.Get();
	auto* NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;

	if (NavMeshPath) {
		Result.Path = Query.PathInstanceToFill;
		NavMeshPath->ResetForRepath();
	}
	else {
		Result.Path = Self->CreatePathInstance<FNavMeshPath>(Query);
		NavPath = Result.Path.Get();
		NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;
	}

	if (!NavMeshPath) {
		UE_LOG(LogGNRecastNavMesh, Error, TEXT("Somehow failed to instantiate destination navpath in FindPath; this should never happen"));
		return Result;
	}

	NavMeshPath->ApplyFlags(Query.NavDataFlags);

	// if travel distance is very small, return the corresponding very short path early
	const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
	if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero()) {
		Result.Path->GetPathPoints().Reset();
		Result.Path->GetPathPoints().Add(FNavPathPoint(AdjustedEndLocation));
		Result.Result = ENavigationQueryResult::Success;

		return Result;
	}

	const float DistanceBudget = Query.CostLimit;
	const FVector StartLocation = FMappingServer::RoundToGrid(Query.StartLocation);
	const FVector EndLocation   = FMappingServer::RoundToGrid(Query.EndLocation);

	const auto [Points, _] = FMappingServer::GetInstance().FindPath(StartLocation, EndLocation, DistanceBudget);

	if (Points.IsEmpty()) {
		Result = ENavigationQueryResult::Fail;
		return Result;
	}
	
	for (const FVector& Point : Points) {
		Result.Path->GetPathPoints().Add(FNavPathPoint(Point));
	}

	if (FVector::Distance(Points.Last(), EndLocation) > 0.1 && !Query.bAllowPartialPaths) {
		Result.Result = ENavigationQueryResult::Fail;
		return Result;
	}

	Result.Path->MarkReady();
	Result.Result = ENavigationQueryResult::Success;

	return Result;
}
