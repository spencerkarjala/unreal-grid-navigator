#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "MapData/NavGridLevel.h"
#include "Display/NavGridRenderingComponent.h"
#include "NavigationGridData.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNavigationDataBlockUpdatedDelegate, uint32, ID, const FBox&, Bounds);

UCLASS()
class GRIDNAVIGATOR_API ANavigationGridData : public ARecastNavMesh
{
	GENERATED_BODY()

	friend class FNavigationGridDataGenerator;
	friend class FNavGridDataSerializer;
	
public:
	ANavigationGridData(const FObjectInitializer& ObjectInitializer);

	virtual void OnNavigationBoundsChanged() override;
	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas) override;
	virtual UPrimitiveComponent* ConstructRenderingComponent() override;
	virtual FBox GetBounds() const override;

	virtual void Serialize(FArchive& Ar) override;

	virtual void ConditionalConstructGenerator() override;

	UFUNCTION(CallInEditor, Category="Navigation", DisplayName="Rebuild Navigation")
	void RebuildNavigation() const;

	FString GetDataString() const;

	FORCEINLINE void UpdateBlockData(const uint32 BlockID, const FBox& NewBoundData);

	TMap<uint32, FNavGridBlock>& GetNavigationBlocks() const;
	FORCEINLINE TSharedPtr<FNavGridLevel> GetLevelData() const;

	FORCEINLINE TArray<NavGrid::FNode> GetNodeList() const;

	UFUNCTION(BlueprintCallable, Category="Navigation", DisplayName="Get Level Data")
	FORCEINLINE FNavGridLevel& GetLevelDataBlueprint() const;

	UPROPERTY(BlueprintAssignable, Category = "Navigation")
	FNavigationDataBlockUpdatedDelegate OnNavigationDataBlockUpdated;

private:
	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	TSharedPtr<FNavGridLevel> LevelData = nullptr;
};
