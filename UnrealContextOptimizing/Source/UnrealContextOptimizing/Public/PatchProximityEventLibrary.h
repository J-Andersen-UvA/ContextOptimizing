#pragma once

#include "CoreMinimal.h"
#include "ContactEventTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PatchProximityEventLibrary.generated.h"

UCLASS()
class UNREALCONTEXTOPTIMIZING_API UPatchProximityEventLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(
        BlueprintPure,
        Category = "Context Optimizing|Source Semantic Events",
        meta = (ClampMin = "1", DisplayName = "Get Patch Proximity Frame Neighbors"))
    static TArray<FPatchProximityFrameNeighborGroup> getPatchProximityFrameNeighbors(
        const FPatchProximityAnimationData& animationData,
        int32 maximumFrameDistance = 1);

    UFUNCTION(
        BlueprintPure,
        Category = "Context Optimizing|Source Semantic Events",
        meta = (DisplayName = "Get Patch Proximity Point Neighbors"))
    static TArray<FPatchProximityPointNeighborGroup> getPatchProximityPointNeighbors(
        const FPatchProximityFrameNeighborGroup& frameNeighbors);

    UFUNCTION(
        BlueprintPure,
        Category = "Context Optimizing|Source Semantic Events",
        meta = (DisplayName = "Build Source Semantic Patch Event"))
    static FSourceSemanticPatchEvent buildSourceSemanticPatchEvent(
        const FPatchProximityPointNeighborGroup& pointNeighbors);

    UFUNCTION(
        BlueprintCallable,
        Category = "Context Optimizing|Source Semantic Events",
        meta = (DisplayName = "Map Source Semantic Patch Event To Target"))
    static FPatchProximityTarget mapSourceSemanticPatchEventToTarget(
        const FSourceSemanticPatchEvent& sourceEvent,
        const FSemanticPose& targetPose,
        const TArray<FSurfacePatchAnchor>& targetPatches,
        double targetNormalOffset = 0.0);

    UFUNCTION(
        BlueprintPure,
        Category = "Context Optimizing|Target Semantic Evaluation",
        meta = (DisplayName = "Evaluate Target Semantic Patch Event"))
    static FTargetSemanticEventEvaluation evaluateTargetSemanticPatchEvent(
        const FSourceSemanticPatchEvent& sourceEvent,
        const FPatchProximityTarget& target,
        const FSemanticPose& targetPose,
        const TArray<FSemanticInteractionPoint>& targetInteractionPoints,
        const FTargetSemanticEventEvaluationSettings& settings);
};
