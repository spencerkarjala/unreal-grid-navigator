#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavGridRenderingComponent.h"
#include "GNRecastNavMesh.generated.h"

UCLASS()
class GRIDNAVIGATOR_API AGNRecastNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

public:
	AGNRecastNavMesh(const FObjectInitializer& ObjectInitializer);

	virtual void OnNavigationBoundsChanged() override;
	virtual UPrimitiveComponent* ConstructRenderingComponent() override;

	virtual void Serialize(FArchive& Ar) override;

	virtual void ConditionalConstructGenerator() override;

	UFUNCTION(CallInEditor, Category="Navigation", DisplayName="Rebuild Navigation")
	void RebuildNavigation() const;

private:
	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	UPROPERTY()
	TObjectPtr<UNavGridRenderingComponent> DebugRenderingComponent;

	void HandleRebuildNavigation() const;
};
