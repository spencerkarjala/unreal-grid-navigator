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

	auto& MapServer = FMappingServer::GetInstance();
	const auto NodeList = MapServer.GetMapNodeList();
	
	for (const auto& Node : NodeList) {
		const FVector BoxPos(Node.X * 100.00, Node.Y * 100.0, Node.Layer * 25.0);
		const FVector BoxDiagonal(2.5, 2.5, 2.5);

		const FBox BoxDims(BoxPos - BoxDiagonal, BoxPos + BoxDiagonal);
		FColor BoxColor(0, 255, 0);
		
		for (const auto& [InNodeID, OutNodeID, EdgeType, EdgeDirection] : Node.OutEdges) {
			const auto& InNodeResult = MapServer.GetNode(InNodeID);   // should never fail
			const auto& OutNodeResult = MapServer.GetNode(OutNodeID); // can fail if something else has broken

			check(InNodeResult.has_value() && InNodeResult->get() == Node);

			FColor LineColor;
			switch(EdgeType) {
			case NavigationMap::EMapEdgeType::None:
				LineColor = FColor(50, 50, 50);
				break;
			case NavigationMap::EMapEdgeType::Direct:
				LineColor = FColor(200, 200, 200);
				break;
			case NavigationMap::EMapEdgeType::Slope:
			case NavigationMap::EMapEdgeType::SlopeTop:
			case NavigationMap::EMapEdgeType::SlopeBottom:
				LineColor = FColor(0, 255, 255);
				break;
			case NavigationMap::EMapEdgeType::CliffDown:
			case NavigationMap::EMapEdgeType::CliffUp:
				LineColor = FColor(70, 0, 70);
				break;
			}
			
			if (!OutNodeResult.has_value()) {
				LineColor = FColor(255, 0, 0);
				BoxColor = FColor(255, 0, 0);
			}

			const FIntVector3 InNodeIndex(Node.X, Node.Y, Node.Layer);
			const FVector InNodeWorldPos = FMappingServer::GridIndexToWorld(InNodeIndex);

			const FIntVector3 OutNodeIndex(Node.X + EdgeDirection.X, Node.Y + EdgeDirection.Y, Node.Layer + EdgeDirection.Z);
			const FVector OutNodeWorldPos = FMappingServer::GridIndexToWorld(OutNodeIndex);

			// slight offset so arrows don't all start and end in the same place; increases readability
			const FVector MidPointWorldPos = (InNodeWorldPos + OutNodeWorldPos) / 2.0;
			const FVector InNodeWorldPosWithOffset = MidPointWorldPos + (InNodeWorldPos - MidPointWorldPos) * 0.7;
			const FVector OutNodeWorldPosWithOffset = MidPointWorldPos + (OutNodeWorldPos - MidPointWorldPos) * 0.7;

			NavGridSceneProxy->ArrowLines.Emplace(InNodeWorldPosWithOffset, OutNodeWorldPosWithOffset, LineColor);
		}
		
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
