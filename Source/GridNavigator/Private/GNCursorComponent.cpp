#include "GNCursorComponent.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Interfaces/IPluginManager.h"

DECLARE_LOG_CATEGORY_CLASS(LogGNCursorComponent, Log, All);

// todo: make default mesh destinations configurable through project settings
UGNCursorComponent::UGNCursorComponent()
{
	DestinationMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GNCursorComponent.TargetMesh"));
	if (!DestinationMeshComponent) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to instantiate TargetMesh for GridNavigatorCursor"));
		return;
	}

	PathComponent = CreateDefaultSubobject<USplineComponent>(TEXT("GNCursorComponent.Path"));
	if (!PathComponent) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to instantiate Path for GridNavigatorCursor"))
		return;
	}

	const auto PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("GridNavigator"));
	if (!PluginBaseDir.IsValid()) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to load base directory for GridNavigator plugin"));
		return;
	}

	const auto PluginContentDir = PluginBaseDir.Get()->GetContentDir();
	const auto DefaultDestinationMeshLocation = FPaths::Combine(PluginContentDir, TEXT("/Script/Engine.StaticMesh'/GridNavigator/SM_GridNavigatorCursorDestination_Default.SM_GridNavigatorCursorDestination_Default'"));
	const auto DefaultPathMeshLocation        = FPaths::Combine(PluginContentDir, TEXT("/Script/Engine.StaticMesh'/GridNavigator/SM_GridNavigatorCursorPath_Default.SM_GridNavigatorCursorPath_Default'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DestinationMesh(*DefaultDestinationMeshLocation);
	if (!DestinationMesh.Succeeded()) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to instantiate default destination mesh at '%s'"), *DefaultDestinationMeshLocation);
		return;
	}
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PathMesh(*DefaultPathMeshLocation);
	if (!PathMesh.Succeeded()) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to instantiate default path mesh at '%s'"), *DefaultPathMeshLocation);
		return;
	}
	PathMeshBase = PathMesh.Object;
	
	DestinationMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DestinationMeshComponent->SetStaticMesh(DestinationMesh.Object);
	DestinationMeshComponent->SetMobility(EComponentMobility::Movable);
	DestinationMeshComponent->SetVisibility(true, true);
}

void UGNCursorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGNCursorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	PathMeshComponents.Empty();
}

bool UGNCursorComponent::UpdatePosition(const FVector& WorldDestination, const FVector& DestNormal)
{
	FVector DestinationRounded = FVector(
		round(WorldDestination.X / 100.0) * 100.0,
		round(WorldDestination.Y / 100.0) * 100.0,
		WorldDestination.Z
	);

	// don't update cursor if destination location hasn't changed
	if ((DestinationRounded - CurrCursorLocation).Length() < TodoDistDeltaThreshold) {
		return true;
	}
	
	const FVector& UpDir = FVector::UpVector;
	FVector NormalDir = DestNormal;
	NormalDir.Normalize();

	// hide cursor if floor slope at new destination is too steep
	const double CosOfUpToNormalAngle = FVector::DotProduct(UpDir, NormalDir);
	if (CosOfUpToNormalAngle <= TodoCosOfMaxInclineAngle) {
		SetVisibility(false);
		return true;
	}

	// perform pathfinding
	const auto* World = GetWorld();
	if (!IsValid(World)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to GetWorld while trying to UpdatePosition"));
		return false;
	}

	const auto* NavSys = Cast<UNavigationSystemV1>(World->GetNavigationSystem());
	if (!IsValid(NavSys)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to retrieve navigation system while trying to UpdatePosition"));
		return false;
	}
	
	auto* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to retrieve OwnerActor when trying to UpdatePosition"));
		return false;
	}
	const FVector& OwnerActorLocation = OwnerActor->GetActorLocation();
	const FVector FloorTraceEnd(OwnerActorLocation.X, OwnerActorLocation.Y, OwnerActorLocation.Z - 1000.0);

	// trace from character position to floor to get pathfinding start location
	FHitResult FloorTraceResult;
	const bool bFloorBelowCharacterFound = World->LineTraceSingleByObjectType(FloorTraceResult, OwnerActorLocation, FloorTraceEnd, ECC_WorldStatic);
	if (!bFloorBelowCharacterFound) {
		UE_LOG(LogGNCursorComponent, Warning, TEXT("Tried to UpdatePosition while character was not above a valid floor"));
		return false;
	}
	
	UNavigationPath* FoundPath = NavSys->FindPathToLocationSynchronously(OwnerActor, FloorTraceResult.Location, DestinationRounded);
	if (FoundPath == nullptr || !FoundPath->IsValid()) {
		return false;
	}
	const auto& PathPoints = FoundPath->PathPoints;

	// hide cursor if destination is not reachable
	if (PathPoints.Num() < 2) {
		SetVisibility(false);
		return true;
	}

	// rotate destination mesh if floor slope is not flat
	FRotator UpVecToNormalRotation(0);
	if (CosOfUpToNormalAngle < 1.0 - UE_KINDA_SMALL_NUMBER) {
		UpVecToNormalRotation.Yaw = FMath::RadiansToDegrees(atan2(NormalDir.Y, NormalDir.X));
		UpVecToNormalRotation.Pitch = FMath::RadiansToDegrees(-acos(CosOfUpToNormalAngle));
		DestinationRounded.Z = floor(DestinationRounded.Z / 50.0) * 50.0 + 25.0;
	}

	bool UpdatePathSuccess = UpdatePath(PathPoints);
	if (!UpdatePathSuccess) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Unknown failure when trying to update PathMeshComponents"));
		return false;
	}
	
	const auto LocalCursorDestinationPosition = DestinationRounded;
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(UpVecToNormalRotation);

	CurrCursorLocation = DestinationRounded;
	SetVisibility(true);

	return true;
}

bool UGNCursorComponent::ShouldUpdatePosition(const FVector& WorldDestination)
{
	FVector DestinationRounded = FVector(
		round(WorldDestination.X / 100.0) * 100.0,
		round(WorldDestination.Y / 100.0) * 100.0,
		WorldDestination.Z
	);
	const float DistFromCurrCursorPosition = (DestinationRounded - CurrCursorLocation).Length();
	return DistFromCurrCursorPosition < TodoDistDeltaThreshold;
}

bool UGNCursorComponent::SetDestinationMesh(UStaticMesh* Mesh)
{
	if (!DestinationMeshComponent) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Tried to SetDestinationMesh without a valid DestinationMeshComponent"));
		return false;
	}
	if (!IsValid(Mesh)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Tried to SetDestinationMesh with an invalid Mesh parameter"));
		return false;
	}
	
	DestinationMeshComponent->SetStaticMesh(Mesh);
	return true;
}

bool UGNCursorComponent::SetPathMesh(UStaticMesh* Mesh)
{
	if (!IsValid(Mesh)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Tried to SetPathMesh with an invalid Mesh parameter"));
		return false;
	}
	for (const TObjectPtr<USplineMeshComponent>& PathMeshComponent : PathMeshComponents) {
		if (!PathMeshComponent) {
			UE_LOG(LogGNCursorComponent, Error, TEXT("Tried to SetPathMesh with at least one invalid PathMeshComponent"));
			return false;
		}
	}
	
	PathMeshBase = Mesh;
	for (const auto& PathMeshComponent : PathMeshComponents) {
		PathMeshComponent->SetStaticMesh(PathMeshBase);
	}

	return true;
}

bool UGNCursorComponent::UpdatePath(const TArray<FVector>& Points)
{
	if (!PathComponent) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Tried to UpdatePath without a valid PathComponent"));
		return false;
	}

	PathComponent->ClearSplinePoints();
	for (const auto& Point : Points) {
		PathComponent->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
	}
	PathComponent->UpdateSpline();

	return UpdatePathMesh();
}

bool UGNCursorComponent::UpdatePathMesh()
{
	if (!PathComponent) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Tried to UpdatePathMesh without a valid PathComponen"));
		return false;
	}

	const int NumSegments = PathComponent->GetNumberOfSplineSegments();
	int NumInstantiatedSegmentComponents = PathMeshComponents.Num();

	// only runs in the case that NumSegments is more than the available number of mesh components available,
	// in which case new mesh components are created and instantiated
	for (int i = NumInstantiatedSegmentComponents; i < NumSegments; ++i) {
		AddNewSplineMeshComponent();
	}
	
	NumInstantiatedSegmentComponents = PathMeshComponents.Num();
	int NumIterations = FMath::Min(NumSegments, NumInstantiatedSegmentComponents);
	const FVector2D PathMeshScale(PathMeshScaleFactor);

	// set up spline mesh components that are already instantiated
	for (int i = 0; i < NumIterations; ++i) {
		auto& PathMeshComponent = PathMeshComponents[i];
		check(PathMeshComponent);
		
		const FSplinePoint& P0 = PathComponent->GetSplinePointAt(i, ESplineCoordinateSpace::World);
		const FSplinePoint& P1 = PathComponent->GetSplinePointAt(i+1, ESplineCoordinateSpace::World);

		PathMeshComponent->SetRelativeLocationAndRotation(FVector(0, 0, 0), FRotator(0, 0, 0));
		PathMeshComponent->SetStartAndEnd(P0.Position, P0.LeaveTangent, P1.Position, P1.ArriveTangent, false);
		PathMeshComponent->SetStartScale(PathMeshScale, false);
		PathMeshComponent->SetEndScale(PathMeshScale, false);
		PathMeshComponent->UpdateMesh();
		PathMeshComponent->SetVisibility(true);
	}

	// reset excess meshes so they disappear when path has fewer segments than previous
	for (int i = NumSegments; i < NumInstantiatedSegmentComponents; ++i) {
		auto& PathMeshComponent = PathMeshComponents[i];
		check(PathMeshComponent);

		PathMeshComponent->SetStartPosition(FVector::ZeroVector, false);
		PathMeshComponent->SetEndPosition(FVector::ZeroVector, false);
		PathMeshComponent->UpdateMesh();
		PathMeshComponent->SetVisibility(false);
	}

	return true;
}

void UGNCursorComponent::AddNewSplineMeshComponent()
{
	const int NewSplineMeshIndex = PathMeshComponents.Num();

	const FString SplineMeshName = FString::Printf(TEXT("GNCursorComponent.PathMesh.%d"), NewSplineMeshIndex);
	auto* NewSplineMesh = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), *SplineMeshName);

	if (!NewSplineMesh) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to initialize PathMeshComponent with id '%s'"), *SplineMeshName);
		return;
	}

	NewSplineMesh->SetMobility(EComponentMobility::Movable);
	NewSplineMesh->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
	NewSplineMesh->RegisterComponent();
	NewSplineMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	NewSplineMesh->SetStaticMesh(PathMeshBase);
	NewSplineMesh->SetAbsolute(true, true, true);
	NewSplineMesh->SetVisibility(false);
	
	PathMeshComponents.Add(NewSplineMesh);
}
