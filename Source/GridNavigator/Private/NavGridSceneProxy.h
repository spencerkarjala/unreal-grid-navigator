#pragma once
#include "DebugRenderSceneProxy.h"

class FNavGridSceneProxy final : public FDebugRenderSceneProxy, public FNoncopyable
{
public:
	// FNavGridSceneProxy(const UPrimitiveComponent* InComponent, FNavMeshSceneProxyData* InProxyData, bool ForceToRender = false);
	FNavGridSceneProxy(const UPrimitiveComponent* InComponent);

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
};
