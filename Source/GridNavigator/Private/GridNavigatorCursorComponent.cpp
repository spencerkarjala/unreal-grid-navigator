#include "GridNavigatorCursorComponent.h"

#include "MappingServer.h"
#include "Components/SplineMeshComponent.h"
#include "Interfaces/IPluginManager.h"

DECLARE_LOG_CATEGORY_CLASS(LogGNCursorComponent, Log, All);

// todo: make default mesh destinations configurable through project settings
UGridNavigatorCursorComponent::UGridNavigatorCursorComponent()
{
	DestinationMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GNCursorComponent.TargetMesh"));
	if (!IsValid(DestinationMeshComponent)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to instantiate TargetMesh for GridNavigatorCursor"));
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

	// start with 8 splines, re-use and add more if needed
	PathMeshComponents.SetNum(8);
	for (int i = 0; i < 8; ++i) {
		FString SplineMeshName = FString::Printf(TEXT("GNCursorComponent.PathMesh.%d"), i);
		PathMeshComponents[i] = CreateDefaultSubobject<USplineMeshComponent>(*SplineMeshName);
		if (!IsValid(PathMeshComponents[i])) {
			UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to initialize PathMeshComponent with id '%s'"), *SplineMeshName);
			return;
		}
	
		PathMeshComponents[i]->SetCollisionResponseToAllChannels(ECR_Ignore);
		PathMeshComponents[i]->SetStaticMesh(PathMeshBase);
		PathMeshComponents[i]->SetVisibility(true);
	}
}

void UGridNavigatorCursorComponent::PostInitProperties()
{
	Super::PostInitProperties();
}

bool UGridNavigatorCursorComponent::UpdatePosition(const FVector& WorldDestination, const FVector& DestNormal)
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
	const auto* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Failed to retrieve OwnerActor when trying to UpdatePosition"));
		return false;
	}
	const TArray<FVector> PathPoints = FMappingServer::GetInstance().FindPath(OwnerActor->GetActorLocation(), DestinationRounded);

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

	bool UpdatePathMeshSuccess = UpdatePathMesh(PathPoints);
	if (!UpdatePathMeshSuccess) {
		UE_LOG(LogGNCursorComponent, Error, TEXT("Unknown failure when trying to update PathMeshComponents"));
		return false;
	}
	
	const auto LocalCursorDestinationPosition = DestinationRounded;
	// const auto LocalCursorDestinationPosition = DestinationRounded - GetOwner->GetActorLocation();
	DestinationMeshComponent->SetRelativeLocation(LocalCursorDestinationPosition);
	DestinationMeshComponent->SetWorldRotation(UpVecToNormalRotation);

	CurrCursorLocation = DestinationRounded;
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
	const int NumSegments = Points.Num() - 1;
	const int NumActivePathMeshComponents = PathMeshComponents.Num();
	const int NumInitialIterations = FMath::Min(NumSegments, NumActivePathMeshComponents);

	// set up spline mesh components that are already instantiated
	for (int i = 0; i < NumInitialIterations; ++i) {
		const auto& P0 = Points[i];
		const auto& P1 = Points[i+1];
		auto& PathMeshComponent = PathMeshComponents[i];
		check(PathMeshComponent->IsValidLowLevel());
		
		PathMeshComponent->SetStartPosition(P0);
		PathMeshComponent->SetEndPosition(P1);
		PathMeshComponent->SetVisibility(true);
	}

	// if there are leftover spline mesh components, disable them
	if (NumSegments < NumActivePathMeshComponents) {
		for (int i = NumSegments; i < NumActivePathMeshComponents; ++i) {
			auto& PathMeshComponent = PathMeshComponents[i];
			check(PathMeshComponent->IsValidLowLevel());

			PathMeshComponent->SetStartPosition(FVector::ZeroVector);
			PathMeshComponent->SetEndPosition(FVector::ZeroVector);
			PathMeshComponent->SetVisibility(false);
		}
	}
	// if there are leftover segments, add new spline mesh components to accommodate
	else if (NumActivePathMeshComponents < NumSegments) {
		for (int i = NumActivePathMeshComponents; i < NumSegments; ++i) {
			const auto& P0 = Points[i];
			const auto& P1 = Points[i+1];

			FString SplineMeshName = FString::Printf(TEXT("GNCursorComponent.PathMesh.%d"), i);
			auto* NewPathMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), *SplineMeshName);
			NewPathMeshComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			NewPathMeshComponent->RegisterComponent();
			NewPathMeshComponent->SetStaticMesh(PathMeshBase);
			NewPathMeshComponent->SetStartPosition(P0);
			NewPathMeshComponent->SetEndPosition(P1);
		}
	} 
	
	return true;
}
