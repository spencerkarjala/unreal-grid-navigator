#include "NavigationGridData.h"

#include "NavigationData.h"
#include "NavigationSystem.h"

#include <functional>

#include "NavGridRenderingComponent.h"
#include "MappingServer.h"
#include "NavMesh/PImplRecastNavMesh.h"
#include "NavigationGridDataGenerator.h"
#include "MapData/NavGridDataSerializer.h"
#include "MapData/NavGridLevel.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavigationGridData, Log, All);

ANavigationGridData::ANavigationGridData(const FObjectInitializer& ObjectInitializer) : ARecastNavMesh(ObjectInitializer)
{
	FindPathImplementation = this->FindPath;
	LevelData = MakeShared<FNavGridLevel>();
}

void ANavigationGridData::OnNavigationBoundsChanged()
{
	// Super::OnNavigationBoundsChanged();
	HandleRebuildNavigation();
}

void ANavigationGridData::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
	Super::RebuildDirtyAreas(DirtyAreas);
}

UPrimitiveComponent* ANavigationGridData::ConstructRenderingComponent()
{
	DebugRenderingComponent = NewObject<UNavGridRenderingComponent>(this, TEXT("NavGridRenderingComponent"), RF_Transient);
	if (!DebugRenderingComponent) {
		return nullptr;
	}
	return DebugRenderingComponent.Get();
}

void ANavigationGridData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	FNavGridDataSerializer::Serialize(Ar, this);
}

void ANavigationGridData::ConditionalConstructGenerator()
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
	
	FNavigationGridDataGenerator* Generator = new FNavigationGridDataGenerator(this);
	if (Generator == nullptr) {
		UE_LOG(LogNavigationGridData, Error, TEXT("Failed to instantiate new GNNavDataGenerator"));
		return;
	}
	NavDataGenerator = MakeShareable(static_cast<FNavDataGenerator*>(Generator));
}

void ANavigationGridData::RebuildNavigation() const
{
	HandleRebuildNavigation();
}

FString ANavigationGridData::GetDataString() const
{
	return LevelData->ToString();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ANavigationGridData::UpdateBlockData(const uint32 BlockID, const FBox& NewBoundData)
{
	LevelData->UpdateBlock(BlockID, FNavGridBlock(NewBoundData));
}

TMap<uint32, FNavGridBlock>& ANavigationGridData::GetNavigationBlocks() const
{
	return LevelData->Blocks;
}

TSharedPtr<FNavGridLevel> ANavigationGridData::GetLevelData() const
{
	return LevelData;
}

FNavGridLevel& ANavigationGridData::GetLevelDataBlueprint() const
{
	return *LevelData;
}

void ANavigationGridData::HandleRebuildNavigation() const
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

FPathFindingResult ANavigationGridData::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	DECLARE_CYCLE_STAT(TEXT("Grid Pathfinding"), STAT_Navigation_GridPathfinding, STATGROUP_Navigation);
	SCOPE_CYCLE_COUNTER(STAT_Navigation_GridPathfinding);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	FPathFindingResult Result(ENavigationQueryResult::Error);

	UE_LOG(LogNavigationGridData, Log, TEXT("Got cost limit of: '%0.2f'"), Query.CostLimit);

	const auto* Self = Cast<const ANavigationGridData>(Query.NavData.Get());
	const auto* NavFilter = Query.QueryFilter.Get();

	if (!Self) {
		UE_LOG(LogNavigationGridData, Error, TEXT("Failed to retrieve reference to RecastNavMesh in FindPath"));
		return Result;
	}
	if (!NavFilter) {
		UE_LOG(LogNavigationGridData, Error, TEXT("Failed to retrieve reference to query filter in FindPath"));
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
		UE_LOG(LogNavigationGridData, Error, TEXT("Somehow failed to instantiate destination navpath in FindPath; this should never happen"));
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

	UE_LOG(LogNavigationGridData, Log, TEXT("FindPath with nav data: %s"), *Self->GetPathName());

	const auto Points = Self->LevelData->FindPath(Query.StartLocation, Query.EndLocation);

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
