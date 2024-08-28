#pragma once

#include "NavigationGridData.h"

class FNavGridDataSerializer
{
	friend class ANavigationGridData;
	
public:
	static void Serialize(FArchive& Ar, ANavigationGridData* NavData);
};
