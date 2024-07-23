#pragma once

#include "CoreMinimal.h"
#include "Debug/DebugDrawComponent.h"
#include "GNNavMeshRenderingComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UGNNavMeshRenderingComponent : public UDebugDrawComponent
{
	GENERATED_BODY()

public:
	// ReSharper disable once CppNonExplicitConvertingConstructor
	UGNNavMeshRenderingComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	virtual FDebugRenderSceneProxy* CreateDebugSceneProxy() override;
	virtual FDebugDrawDelegateHelper& GetDebugDrawDelegateHelper() override;

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
	bool CheckShowNavigationFlag() const;
};
