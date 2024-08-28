#include "Display/NavGridSceneProxy.h"

FNavGridSceneProxy::FNavGridSceneProxy(const UPrimitiveComponent* InComponent)
	: FDebugRenderSceneProxy(InComponent)
{
	DrawType = SolidAndWireMeshes;
	ViewFlagName = TEXT("Navigation");
}

FPrimitiveViewRelevance FNavGridSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance ViewRelevance;

	ViewRelevance.bDrawRelevance = true;
	ViewRelevance.bDynamicRelevance = true;
	ViewRelevance.bNormalTranslucency = true;
	ViewRelevance.bShadowRelevance = true;
	ViewRelevance.bEditorPrimitiveRelevance = true;
	
	return ViewRelevance; 
}

void FNavGridSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	FDebugRenderSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);
}
