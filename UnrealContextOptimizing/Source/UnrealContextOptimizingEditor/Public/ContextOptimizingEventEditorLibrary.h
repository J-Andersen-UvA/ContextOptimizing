#pragma once

#include "ContactEventTypes.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SemanticPatchProximityTypes.h"
#include "ContextOptimizingEventEditorLibrary.generated.h"

class AActor;
class UAnimSequenceBase;

UCLASS()
class UNREALCONTEXTOPTIMIZINGEDITOR_API UContextOptimizingEventEditorLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(
        BlueprintCallable,
        Category = "Context Optimizing|Source Semantic Events",
        meta = (
            DisplayName = "Extract And Map Semantic Patch Events",
            AdvancedDisplay = "targetNormalOffset"))
    static TArray<FMappedSemanticPatchEvent> extractAndMapSemanticPatchEvents(
        const UAnimSequenceBase* sourceAnimation,
        const AActor* sourceActor,
        const UAnimSequenceBase* targetAnimation,
        const AActor* targetActor,
        const FPatchProximitySettings& proximitySettings,
        const FTargetSemanticEventEvaluationSettings& targetEvaluationSettings,
        int32 maximumFrameDistance = 1,
        double targetNormalOffset = 0.0);
};
