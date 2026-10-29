#pragma once

#include "ContextOptimizationPipelineTypes.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ContextOptimizationPipelineEditorLibrary.generated.h"

class AActor;
class UAnimSequence;
class UAnimSequenceBase;

UCLASS()
class UNREALCONTEXTOPTIMIZINGEDITOR_API UContextOptimizationPipelineEditorLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(
        BlueprintCallable,
        Category = "Context Optimizing|Pipeline",
        meta = (DisplayName = "Bake Semantic Event Corrections"))
    static FContextOptimizationPipelineResult bakeSemanticEventCorrections(
        const UAnimSequenceBase* sourceAnimation,
        const AActor* sourceActor,
        UAnimSequence* initialTargetAnimation,
        const AActor* targetActor,
        const FContextOptimizationPipelineSettings& settings);
};
