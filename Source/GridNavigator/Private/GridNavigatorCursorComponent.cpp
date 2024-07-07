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
