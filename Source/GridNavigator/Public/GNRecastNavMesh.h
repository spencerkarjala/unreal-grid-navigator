#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "GNRecastNavMesh.generated.h"

UCLASS()
class GRIDNAVIGATOR_API AGNRecastNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

	AGNRecastNavMesh(const FObjectInitializer& ObjectInitializer);

	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
};
