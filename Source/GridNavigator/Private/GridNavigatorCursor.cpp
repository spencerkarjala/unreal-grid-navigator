#include "GridNavigatorCursor.h"

#include "Components/SplineMeshComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogGridNavigatorCursor, Log, All);

AGridNavigatorCursor::AGridNavigatorCursor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("GridNavigatorCursor.Root"));
	if (!IsValid(RootComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate RootComponent for GridNavigatorCursor"));
		return;
	}

	DestinationMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopDownViewCursor.TargetMesh"));
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate TargetMesh for GridNavigatorCursor"));
	}
	else {
		DestinationMeshComponent->SetupAttachment(RootComponent);
		// DestinationMeshComponent->SetStaticMesh();
		DestinationMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	PathMeshComponent = CreateDefaultSubobject<USplineMeshComponent>(TEXT("TopDownViewCursor.PathMesh"));
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate PathMesh for GridNavigatorCursor"));
	}
	else {
		PathMeshComponent->SetupAttachment(RootComponent);
		// PathMeshComponent->SetStaticMesh();
		PathMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AGridNavigatorCursor::SetVisibility(const bool bIsVisible)
{
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Tried to SetVisibility on GridNavigatorCursor with no valid destination mesh"));
		return;
	}
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Tried to SetVisibility on GridNavigatorCursor with no valid path mesh"));
		return;
	}

	DestinationMeshComponent->SetVisibility(bIsVisible, false);
	PathMeshComponent->SetVisibility(bIsVisible, false);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AGridNavigatorCursor::UpdatePosition(const FVector& NewWorldPosition, const FRotator& NewOrientation)
{
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Tried to UpdatePosition on GridNavigatorCursor with no valid destination mesh"));
		return;
	}
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Tried to UpdatePosition on GridNavigatorCursor with no valid path mesh"));
		return;
	}

	const auto LocalCursorDestinationPosition = NewWorldPosition - GetActorLocation();
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(NewOrientation);

	// todo: fill out path spline mesh
}
