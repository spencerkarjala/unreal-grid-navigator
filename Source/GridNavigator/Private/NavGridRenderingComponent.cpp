#include "NavGridRenderingComponent.h"

#include "GNRecastNavMesh.h"
#include "MappingServer.h"
#include "NavGridSceneProxy.h"
#include "NavMesh/NavMeshRenderingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridRenderingComponent, Log, All);

UNavGridRenderingComponent::UNavGridRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseEditorCompositing = true;
	SetHiddenInGame(false);
	SetVisibility(true);
}

void UNavGridRenderingComponent::OnRegister()
{
	Super::OnRegister();

	// disable collision entirely
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionProfileName(FName("NoCollision"));
	SetCollisionObjectType(ECC_GameTraceChannel1);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetSimulatePhysics(false);
	SetEnableGravity(false);
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyInstance.SetResponseToAllChannels(ECR_Ignore);

	const bool bIsInEditor = GEngine ? GEngine->IsEditor() : false;
	if (!bIsInEditor) {
		return;
	}

	const auto* World = GetWorld();
	if (!World) {
		UE_LOG(LogNavGridRenderingComponent, Error, TEXT("Received null world reference while registering"));
		return;
	}
	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(
		CheckRenderNavigationFlagTimer,
		this,
		&UNavGridRenderingComponent::CheckRenderNavigationFlagActive,
		0.1,
		true,
		0.0
	);
}

void UNavGridRenderingComponent::OnUnregister()
{
	Super::OnUnregister();
	
	bPrevShowNavigationFlagValue = false;
}

FDebugRenderSceneProxy* UNavGridRenderingComponent::CreateDebugSceneProxy()
{
	const bool ShouldShowNavigation = CheckShowNavigationFlag();
	if (!ShouldShowNavigation) {
		return nullptr;
	}

	const auto* NavGrid = Cast<AGNRecastNavMesh>(GetOwner());
	if (!IsValid(NavGrid) || !NavGrid->IsDrawingEnabled() || !IsVisible()) {
		return nullptr;
	}

	auto* NavGridSceneProxy = new FNavGridSceneProxy(this);
	if (!NavGridSceneProxy) {
		return nullptr;
	}
	
	const auto NodeList = FMappingServer::GetInstance().GetMapNodeList();
	
	for (const auto& Node : NodeList) {
		const FVector BoxPos(Node.X * 100.00, Node.Y * 100.0, Node.Layer * 25.0);
		const FVector BoxDiagonal(2.5, 2.5, 2.5);

		const FBox BoxDims(BoxPos - BoxDiagonal, BoxPos + BoxDiagonal);
		FColor BoxColor(0, 255, 0);
		
		NavGridSceneProxy->Boxes.Emplace(BoxDims, BoxColor);
	}

	this->SetVisibility(true);
	
	return NavGridSceneProxy;
}

FBoxSphereBounds UNavGridRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const auto CalculatedBounds = FMappingServer::GetInstance().GetBounds().TransformBy(LocalToWorld);
	return CalculatedBounds;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool UNavGridRenderingComponent::CheckShowNavigationFlag() const
{
	const bool bIsEditor = IsValid(GEngine) ? GEngine->IsEditor() : false;
	if (!bIsEditor) {
		return false;
	}

	const auto* Viewport = GEditor ? GEditor->GetActiveViewport() : nullptr;
	auto* Client = Viewport ? Viewport->GetClient() : nullptr;
	const auto* Flags = Client ? Client->GetEngineShowFlags() : nullptr;
	if (!Flags) {
		return false;
	}

	return Flags->GetSingleFlag(FEngineShowFlags::EShowFlag::SF_Navigation);
}

void UNavGridRenderingComponent::CheckRenderNavigationFlagActive()
{
	const bool bShouldShowNavigation = CheckShowNavigationFlag();
	if (bShouldShowNavigation != bPrevShowNavigationFlagValue) {
		MarkRenderStateDirty();
	}
	bPrevShowNavigationFlagValue = bShouldShowNavigation;
}
