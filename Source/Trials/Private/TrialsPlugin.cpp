// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Trials.h"

#include "Core.h"
#include "Engine.h"
#include "ModuleManager.h"
#include "ModuleInterface.h"

#include "WebSocketsModule.h"

class FTrialsPlugin : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FTrialsPlugin, Trials )

void FTrialsPlugin::StartupModule()
{
    FModuleManager::LoadModuleChecked<FWebSocketsModule>("WebSockets");

}

void FTrialsPlugin::ShutdownModule()
{
	
}