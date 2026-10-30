#include "SemanticInteractionPointsComponent.h"

#include "GameFramework/Actor.h"
#include "SemanticInteractionPointComponent.h"

USemanticInteractionPointsComponent::USemanticInteractionPointsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

TArray<USemanticInteractionPointComponent*>
USemanticInteractionPointsComponent::getInteractionPointComponents() const
{
    TArray<USemanticInteractionPointComponent*> points;
    if (const AActor* owner = GetOwner())
    {
        owner->GetComponents(points);
    }
    if (points.IsEmpty())
    {
        for (USemanticInteractionPointComponent* point : interactionPoints)
        {
            if (point != nullptr)
            {
                points.Add(point);
            }
        }
    }
    return points;
}

TArray<FSemanticInteractionPoint>
USemanticInteractionPointsComponent::getEnabledInteractionPoints() const
{
    TArray<FSemanticInteractionPoint> points;
    for (const USemanticInteractionPointComponent* component :
         getInteractionPointComponents())
    {
        if (component != nullptr && component->enabled)
        {
            points.Add(component->toInteractionPoint());
        }
    }
    return points;
}

bool USemanticInteractionPointsComponent::hasPoint(const FName pointId) const
{
    return getInteractionPointComponents().ContainsByPredicate(
        [pointId](const USemanticInteractionPointComponent* point)
        {
            return point != nullptr && point->pointId == pointId;
        });
}
