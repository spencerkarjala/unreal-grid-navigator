#include "GNNavMeshRenderingComponent.h"

#include "MappingServer.h"
#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"

UGNNavMeshRenderingComponent::UGNNavMeshRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseEditorCompositing = true;
}

void UGNNavMeshRenderingComponent::OnRegister()
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
}

void UGNNavMeshRenderingComponent::OnUnregister()
{
	Super::OnUnregister();
}

FDebugRenderSceneProxy* UGNNavMeshRenderingComponent::CreateDebugSceneProxy()
{
	const bool ShouldShowNavigation = CheckShowNavigationFlag();
	if (!ShouldShowNavigation) {
		return nullptr;
	}
	
	return nullptr;
}

FDebugDrawDelegateHelper& UGNNavMeshRenderingComponent::GetDebugDrawDelegateHelper()
{
	return Super::GetDebugDrawDelegateHelper();
}

FBoxSphereBounds UGNNavMeshRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FMappingServer::GetInstance().GetBounds().TransformBy(LocalToWorld);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool UGNNavMeshRenderingComponent::CheckShowNavigationFlag() const
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
