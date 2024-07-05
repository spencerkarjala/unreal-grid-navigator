#include "GridNavigatorCursor.h"

DECLARE_LOG_CATEGORY_CLASS(LogGridNavigatorCursor, Log, All);

AGridNavigatorCursor::AGridNavigatorCursor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("GridNavigatorCursor.Root"));
	if (!IsValid(RootComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate RootComponent for GridNavigatorCursor"));
		return;
	}

	TargetMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopDownViewCursor.TargetMesh"));
	if (!IsValid(TargetMeshComponent)) {
		UE_LOG(LogGridNavigatorCursor, Error, TEXT("Failed to instantiate TargetMesh for GridNavigatorCursor"));
	}
	else {
		MeshComponent->SetupAttachment(RootComponent);
		// todo: set static mesh
		TargetMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

void AGridNavigatorCursor::BeginPlay()
{
	Super::BeginPlay();
}
