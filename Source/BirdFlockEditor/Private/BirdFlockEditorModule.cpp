#include "BirdFlockEditorModule.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "BirdFlockEditor"

void FBirdFlockEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Adds a "Birds" shortcut button to the Details panel category strip for actors.
	// Mirrors how the engine registers General/Actor/LOD/etc. in DetailCustomizations.
	// Добавляет кнопку "Birds" в полосу категорий панели Details для акторов.
	TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("Actor", "Birds",
		LOCTEXT("BirdsSection", "Birds"));
	Section->AddCategory("Birds");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBirdFlockEditorModule, BirdFlockEditor)
