#include "GridNavigator.h"

#define LOCTEXT_NAMESPACE "FGridNavigatorModule"

// code that executes after the module is loaded into memory; this timing can be adjusted in the .uplugin file
void FGridNavigatorModule::StartupModule()
{
}

// may be called by the engine for cleanup (eg. for modules with dynamic reloading, is called before unloading)
void FGridNavigatorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGridNavigatorModule, GridNavigator)