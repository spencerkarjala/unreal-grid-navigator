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
	DestinationMeshComponent->SetVisibility(true, true);
	
	PathMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	PathMeshComponent->SetStaticMesh(PathMesh.Object);
	PathMeshComponent->SetVisibility(true, true);
}

bool UGridNavigatorCursorComponent::UpdatePosition(const FVector& WorldDestination, const FVector& DestNormal)
{
	FVector DestinationRounded = FVector(
		round(WorldDestination.X / 100.0) * 100.0,
		round(WorldDestination.Y / 100.0) * 100.0,
		WorldDestination.Z
	);

	if ((DestinationRounded - CurrCursorLocation).Length() < TodoDistDeltaThreshold) {
		return true;
	}
	CurrCursorLocation = DestinationRounded;
	
	const FVector UpDir(0.0, 0.0, 1.0);
	FVector HitNormal = DestNormal;
	HitNormal.Normalize();

	const double CosOfUpToNormalAngle = FVector::DotProduct(UpDir, HitNormal);

	if (CosOfUpToNormalAngle <= TodoCosOfMaxInclineAngle) {
		SetVisibility(false);
		return true;
	}
	
	FRotator UpVecToNormalRotation;
	if (CosOfUpToNormalAngle < 1.0 - UE_KINDA_SMALL_NUMBER) {
		UpVecToNormalRotation.Yaw = FMath::RadiansToDegrees(atan2(HitNormal.Y, HitNormal.X));
		UpVecToNormalRotation.Pitch = FMath::RadiansToDegrees(-acos(CosOfUpToNormalAngle));
		DestinationRounded.Z = floor(DestinationRounded.Z / 50.0) * 50.0 + 25.0;
	}
	
	const auto LocalCursorDestinationPosition = DestinationRounded;
	// const auto LocalCursorDestinationPosition = HitLocationRounded - ParentActor->GetActorLocation();
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(UpVecToNormalRotation);
	
	SetVisibility(true);

	return true;

	// todo: fill out path spline mesh
}

bool UGridNavigatorCursorComponent::ShouldUpdatePosition(const FVector& WorldDestination)
{
	FVector DestinationRounded = FVector(
		round(WorldDestination.X / 100.0) * 100.0,
		round(WorldDestination.Y / 100.0) * 100.0,
		WorldDestination.Z
	);
	const float DistFromCurrCursorPosition = (DestinationRounded - CurrCursorLocation).Length();
	return DistFromCurrCursorPosition < TodoDistDeltaThreshold;
}
