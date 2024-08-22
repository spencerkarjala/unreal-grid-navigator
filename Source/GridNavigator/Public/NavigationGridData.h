#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "MapData/NavGridLevel.h"
#include "NavGridRenderingComponent.h"
#include "NavigationGridData.generated.h"

// forward declare private module class
namespace NavGrid { class FLevel; }

UCLASS()
class GRIDNAVIGATOR_API ANavigationGridData : public ARecastNavMesh
{
	GENERATED_BODY()

	friend class FNavigationGridDataGenerator;
	
public:
	ANavigationGridData(const FObjectInitializer& ObjectInitializer);

	virtual void OnNavigationBoundsChanged() override;
	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas) override;
	virtual UPrimitiveComponent* ConstructRenderingComponent() override;

	virtual void Serialize(FArchive& Ar) override;

	virtual void ConditionalConstructGenerator() override;

	UFUNCTION(CallInEditor, Category="Navigation", DisplayName="Rebuild Navigation")
	void RebuildNavigation() const;

	FString GetDataString() const;

private:
	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	UPROPERTY()
	TObjectPtr<UNavGridRenderingComponent> DebugRenderingComponent;

	void HandleRebuildNavigation() const;

	TSharedPtr<FNavGridLevel> LevelData = nullptr;
};
