#include "GridNavigatorCursorComponent.h"

#include "ProceduralMeshComponent.h"
#include "Components/SplineComponent.h"
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

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("TopDownViewCursor.Path"));
	if (!IsValid(SplineComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate PathMesh for GridNavigatorCursor"));
		return;
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
	PathMeshBase = PathMesh.Object;
	
	DestinationMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DestinationMeshComponent->SetStaticMesh(DestinationMesh.Object);
	DestinationMeshComponent->SetVisibility(true, true);

}

void UGridNavigatorCursorComponent::PostInitProperties()
{
	Super::PostInitProperties();
}

bool UGridNavigatorCursorComponent::UpdatePosition(const TArray<FVector>& PathPoints, const FVector& WorldDestination, const FVector& DestNormal)
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
	
	const FVector& UpDir = FVector::UpVector;
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

	UpdatePathMesh(PathPoints);
	
	const auto LocalCursorDestinationPosition = DestinationRounded;
	// const auto LocalCursorDestinationPosition = DestinationRounded - GetOwner->GetActorLocation();
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(UpVecToNormalRotation);

	SetVisibility(true);

	return true;
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

bool UGridNavigatorCursorComponent::UpdatePathMesh(const TArray<FVector>& Points)
{
	for (int i = PathMeshComponents.Num() - 1; i >= 0; --i) {
		auto& Component = PathMeshComponents[i];
		Component->UnregisterComponent();
		PathMeshComponents.RemoveAt(i);
	}
	
	const int NumPoints = Points.Num();
	for (int i = 1; i < NumPoints-1; ++i) {
		USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), NAME_None, RF_Transient);
		NewSplineMeshComponent->SetMobility(EComponentMobility::Movable);
		NewSplineMeshComponent->SetupAttachment(this);
		NewSplineMeshComponent->RegisterComponent();
		NewSplineMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		NewSplineMeshComponent->SetStaticMesh(PathMeshBase);
		NewSplineMeshComponent->SetVisibility(true, true);

		PathMeshComponents.Add(NewSplineMeshComponent);

		NewSplineMeshComponent->SetStartPosition(Points[i-1], false);
		NewSplineMeshComponent->SetEndPosition(Points[i], true);
	}
	
	return true;
}
