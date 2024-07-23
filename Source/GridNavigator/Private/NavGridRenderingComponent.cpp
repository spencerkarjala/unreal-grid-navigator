#include "NavGridRenderingComponent.h"

#include "MappingServer.h"

DECLARE_LOG_CATEGORY_CLASS(LogNavGridRenderingComponent, Log, All);

UNavGridRenderingComponent::UNavGridRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseEditorCompositing = true;
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
		0.2,
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
	
	return nullptr;
}

FDebugDrawDelegateHelper& UNavGridRenderingComponent::GetDebugDrawDelegateHelper()
{
	return Super::GetDebugDrawDelegateHelper();
}

FBoxSphereBounds UNavGridRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FMappingServer::GetInstance().GetBounds().TransformBy(LocalToWorld);
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
	bool bShouldShowNavigation = CheckShowNavigationFlag();
	if (bShouldShowNavigation && !bPrevShowNavigationFlagValue) {
		UE_LOG(LogNavGridRenderingComponent, Error, TEXT("made it here !!"));
		// do something
	}
	bPrevShowNavigationFlagValue = bShouldShowNavigation;
}
