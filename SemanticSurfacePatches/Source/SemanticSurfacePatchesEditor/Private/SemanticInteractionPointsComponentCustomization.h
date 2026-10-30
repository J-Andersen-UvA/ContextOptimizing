#pragma once

#include "IDetailCustomization.h"

class USemanticInteractionPointsComponent;

class FSemanticInteractionPointsComponentCustomization
    : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> makeInstance();

    virtual void CustomizeDetails(
        IDetailLayoutBuilder& detailBuilder) override;

private:
    FReply copyRightToLeft();

    TWeakObjectPtr<USemanticInteractionPointsComponent> component;
};
