#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Editor-side registrations for BirdFlock (Details panel section button etc.).
 *  Редакторная часть BirdFlock (кнопка секции Birds в панели Details и т.п.). */
class FBirdFlockEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
};
