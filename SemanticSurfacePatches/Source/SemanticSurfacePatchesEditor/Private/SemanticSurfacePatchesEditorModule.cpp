#include "Modules/ModuleManager.h"

#include "PropertyEditorModule.h"
#include "SemanticInteractionPointsComponent.h"
#include "SemanticInteractionPointsComponentCustomization.h"
#include "SurfacePatchAnchorComponent.h"
#include "SurfacePatchAnchorComponentCustomization.h"
#include "SurfacePatchesComponent.h"
#include "SurfacePatchesComponentCustomization.h"

class FSemanticSurfacePatchesEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FPropertyEditorModule& propertyEditor =
            FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
                TEXT("PropertyEditor"));
        propertyEditor.RegisterCustomClassLayout(
            USemanticInteractionPointsComponent::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateStatic(
                &FSemanticInteractionPointsComponentCustomization::makeInstance));
        propertyEditor.RegisterCustomClassLayout(
            USurfacePatchesComponent::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateStatic(
                &FSurfacePatchesComponentCustomization::makeInstance));
        propertyEditor.RegisterCustomClassLayout(
            USurfacePatchAnchorComponent::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateStatic(
                &FSurfacePatchAnchorComponentCustomization::makeInstance));
        propertyEditor.NotifyCustomizationModuleChanged();

    }

    virtual void ShutdownModule() override
    {
        if (FPropertyEditorModule* propertyEditor =
            FModuleManager::GetModulePtr<FPropertyEditorModule>(
                TEXT("PropertyEditor")))
        {
            propertyEditor->UnregisterCustomClassLayout(
                USemanticInteractionPointsComponent::StaticClass()->GetFName());
            propertyEditor->UnregisterCustomClassLayout(
                USurfacePatchesComponent::StaticClass()->GetFName());
            propertyEditor->UnregisterCustomClassLayout(
                USurfacePatchAnchorComponent::StaticClass()->GetFName());
            propertyEditor->NotifyCustomizationModuleChanged();
        }
    }

};

IMPLEMENT_MODULE(
    FSemanticSurfacePatchesEditorModule,
    SemanticSurfacePatchesEditor)
