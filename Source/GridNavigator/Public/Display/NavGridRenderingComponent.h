#pragma once

#include "CoreMinimal.h"
#include "Debug/DebugDrawComponent.h"
#include "NavGridRenderingComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNavGridRenderingComponent : public UDebugDrawComponent
{
	GENERATED_BODY()

public:
	explicit UNavGridRenderingComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	virtual FDebugRenderSceneProxy* CreateDebugSceneProxy() override;

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
	bool CheckShowNavigationFlag() const;
	void CheckRenderNavigationFlagActive(); 

	FTimerHandle CheckRenderNavigationFlagTimer;

	bool bPrevShowNavigationFlagValue = false;
};
