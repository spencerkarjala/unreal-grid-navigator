#include "GridNavigatorCursorComponent.h"

#include "Components/SplineMeshComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogGridNavigatorCursor, Log, All);

UGridNavigatorCursorComponent::UGridNavigatorCursorComponent()
{
	DestinationMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopDownViewCursor.TargetMesh"));
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate TargetMesh for GridNavigatorCursor"));
	}
	else {
		// DestinationMeshComponent->SetStaticMesh();
		DestinationMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	PathMeshComponent = CreateDefaultSubobject<USplineMeshComponent>(TEXT("TopDownViewCursor.PathMesh"));
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate PathMesh for GridNavigatorCursor"));
	}
	else {
		// PathMeshComponent->SetStaticMesh();
		PathMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UGridNavigatorCursorComponent::SetVisibility(const bool bIsVisible)
{
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Warning, TEXT("Tried to SetVisibility on GridNavigatorCursor with no valid destination mesh"));
		return;
	}
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Warning, TEXT("Tried to SetVisibility on GridNavigatorCursor with no valid path mesh"));
		return;
	}

	DestinationMeshComponent->SetVisibility(bIsVisible, false);
	PathMeshComponent->SetVisibility(bIsVisible, false);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UGridNavigatorCursorComponent::UpdatePosition(const FVector& NewWorldPosition, const FRotator& NewOrientation)
{
	const auto* ParentActor = GetOwner();
	if (!IsValid(ParentActor)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Tried to UpdatePosition on GridNavigatorCursorComponent with no parent actor"));
		return;
	}
	
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Warning, TEXT("Tried to UpdatePosition on GridNavigatorCursor with no destination mesh"));
		return;
	}
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Warning, TEXT("Tried to UpdatePosition on GridNavigatorCursor with no path mesh"));
		return;
	}

	const auto LocalCursorDestinationPosition = NewWorldPosition - ParentActor->GetActorLocation();
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(NewOrientation);

	// todo: fill out path spline mesh
}
