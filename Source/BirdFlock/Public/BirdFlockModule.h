#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBirdFlock, Log, All);

class FBirdFlockModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
};
