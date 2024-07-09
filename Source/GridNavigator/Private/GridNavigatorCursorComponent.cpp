#include "GridNavigatorCursorComponent.h"

#include "Components/SplineMeshComponent.h"
#include "Interfaces/IPluginManager.h"

DECLARE_LOG_CATEGORY_CLASS(LogGridNavigatorCursor, Log, All);

// todo: make default mesh destinations configurable through project settings
UGridNavigatorCursorComponent::UGridNavigatorCursorComponent()
{
	DestinationMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopDownViewCursor.TargetMesh"));
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate TargetMesh for GridNavigatorCursor"));
		return;
	}

	PathMeshComponent = CreateDefaultSubobject<USplineMeshComponent>(TEXT("TopDownViewCursor.PathMesh"));
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate PathMesh for GridNavigatorCursor"));
	}

	const auto PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("GridNavigator"));
	if (!PluginBaseDir.IsValid()) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to load base directory for GridNavigator plugin"));
		return;
	}

	const auto PluginContentDir = PluginBaseDir.Get()->GetContentDir();
	const auto DefaultDestinationMeshLocation = FPaths::Combine(PluginContentDir, TEXT("/Script/Engine.StaticMesh'/GridNavigator/SM_GridNavigatorCursorDestination_Default.SM_GridNavigatorCursorDestination_Default'"));
	const auto DefaultPathMeshLocation        = FPaths::Combine(PluginContentDir, TEXT("/Script/Engine.StaticMesh'/GridNavigator/SM_GridNavigatorCursorPath_Default.SM_GridNavigatorCursorPath_Default'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DestinationMesh(*DefaultDestinationMeshLocation);
	if (!DestinationMesh.Succeeded()) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate default destination mesh at '%s'"), *DefaultDestinationMeshLocation);
		return;
	}
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PathMesh(*DefaultPathMeshLocation);
	if (!PathMesh.Succeeded()) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate default path mesh at '%s'"), *DefaultPathMeshLocation);
		return;
	}
	
	DestinationMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DestinationMeshComponent->SetStaticMesh(DestinationMesh.Object);
	
	PathMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	PathMeshComponent->SetStaticMesh(PathMesh.Object);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UGridNavigatorCursorComponent::UpdateVisibility(const bool NewVisibility)
{
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Warning, TEXT("Tried to SetIsActive on GridNavigatorCursor with no valid destination mesh"));
		return;
	}
	if (!IsValid(PathMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Warning, TEXT("Tried to SetIsActive on GridNavigatorCursor with no valid path mesh"));
		return;
	}

	DestinationMeshComponent->SetVisibility(NewVisibility, false);
	PathMeshComponent->SetVisibility(NewVisibility, false);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UGridNavigatorCursorComponent::UpdatePosition(const FHitResult& HitResult)
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
	if (!HitResult.IsValidBlockingHit()) {
		UpdateVisibility(false);
		return;
	}

	FVector HitLocationRounded = FVector(
		round(HitResult.Location.X / 100.0) * 100.0,
		round(HitResult.Location.Y / 100.0) * 100.0,
		HitResult.Location.Z
	);

	if ((HitLocationRounded - CurrCursorLocation).Length() < 2e-4) {
		return;
	}
	CurrCursorLocation = HitLocationRounded;
	
	const FVector UpDir(0.0, 0.0, 1.0);
	FVector HitNormal = HitResult.Normal;
	HitNormal.Normalize();

	const double CosOfUpToNormalAngle = FVector::DotProduct(UpDir, HitNormal);

	if (CosOfUpToNormalAngle <= TodoCosOfMaxInclineAngle) {
		UpdateVisibility(false);
		return;
	}
	
	FRotator UpVecToNormalRotation;
	if (CosOfUpToNormalAngle < 1.0 - UE_KINDA_SMALL_NUMBER) {
		UpVecToNormalRotation.Yaw = FMath::RadiansToDegrees(atan2(HitNormal.Y, HitNormal.X));
		UpVecToNormalRotation.Pitch = FMath::RadiansToDegrees(-acos(CosOfUpToNormalAngle));
		HitLocationRounded.Z = floor(HitResult.Location.Z / 50.0) * 50.0 + 25.0;
	}
	
	const auto LocalCursorDestinationPosition = HitLocationRounded;
	// const auto LocalCursorDestinationPosition = HitLocationRounded - ParentActor->GetActorLocation();
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(UpVecToNormalRotation);
	
	UpdateVisibility(true);

	// todo: fill out path spline mesh
}
