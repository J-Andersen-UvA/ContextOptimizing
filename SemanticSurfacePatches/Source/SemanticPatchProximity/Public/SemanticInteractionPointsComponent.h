#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SemanticPatchProximityTypes.h"
#include "SemanticInteractionPointsComponent.generated.h"

class USemanticInteractionPointComponent;

UCLASS(
    BlueprintType,
    ClassGroup = (SemanticPatchProximity),
    meta = (BlueprintSpawnableComponent))
class SEMANTICPATCHPROXIMITY_API USemanticInteractionPointsComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    USemanticInteractionPointsComponent();

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Semantic Interaction Points")
    TArray<TObjectPtr<USemanticInteractionPointComponent>> interactionPoints;

    UFUNCTION(BlueprintPure, Category = "Semantic Interaction Points")
    TArray<USemanticInteractionPointComponent*>
    getInteractionPointComponents() const;

    UFUNCTION(BlueprintPure, Category = "Semantic Interaction Points")
    TArray<FSemanticInteractionPoint> getEnabledInteractionPoints() const;

    UFUNCTION(BlueprintPure, Category = "Semantic Interaction Points")
    bool hasPoint(FName pointId) const;
};
