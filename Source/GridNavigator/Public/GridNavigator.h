#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGridNavigatorModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
