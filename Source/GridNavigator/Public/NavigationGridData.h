#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "MapData/NavGridLevel.h"
#include "NavGridRenderingComponent.h"
#include "NavigationGridData.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNavigationDataBlockUpdatedDelegate, uint32, ID, const FBox&, Bounds);

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

	FORCEINLINE void UpdateBlockData(const uint32 BlockID, const FBox& NewBoundData) const;

	UPROPERTY(BlueprintAssignable, Category = "Navigation")
	FNavigationDataBlockUpdatedDelegate OnNavigationDataBlockUpdated;

private:
	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	UPROPERTY()
	TObjectPtr<UNavGridRenderingComponent> DebugRenderingComponent;

	void HandleRebuildNavigation() const;

	TSharedPtr<FNavGridLevel> LevelData = nullptr;
};
