#include "GNMappingServerSubsystem.h"

#include "MappingServer.h"

bool UGNMappingServerSubsystem::RequestPath(const FVector& FromPoint, const FVector& ToPoint, TArray<FVector>& OutPoints)
{
	OutPoints = FMappingServer::GetInstance().FindPath(FromPoint, ToPoint);
	return true;
}
