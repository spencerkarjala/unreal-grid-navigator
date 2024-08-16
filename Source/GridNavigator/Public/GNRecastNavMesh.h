#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavGridRenderingComponent.h"
#include "GNRecastNavMesh.generated.h"

// forward declare private module class
namespace NavGrid { class FLevel; }

UCLASS()
class GRIDNAVIGATOR_API AGNRecastNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

	friend class FGNNavDataGenerator;
	
public:
	AGNRecastNavMesh(const FObjectInitializer& ObjectInitializer);

	virtual void OnNavigationBoundsChanged() override;
	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas) override;
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

	TSharedPtr<NavGrid::FLevel> LevelData = nullptr;
};
