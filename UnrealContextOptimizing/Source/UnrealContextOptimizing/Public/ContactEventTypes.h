#pragma once

#include "CoreMinimal.h"
#include "SemanticPatchProximityTypes.h"
#include "ContactEventTypes.generated.h"

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FPatchProximityFrameNeighborGroup
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 startFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 endFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    TArray<FPatchProximityFrame> frames;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FPatchProximityPointNeighborGroup
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FName pointId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FName interactionGroupId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 startFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 endFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    TArray<FPatchProximityFrame> frames;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FSourceSemanticPatchEvent
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    bool isValid = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FName pointId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FName interactionGroupId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FName patchId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 startFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 representativeFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event", meta = (Units = "s"))
    double representativeTimeSeconds = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 endFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector2D normalizedPatchLocation = FVector2D::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event", meta = (Units = "cm"))
    double signedNormalOffset = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector componentSpaceInteractionPoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector componentSpacePatchPoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector componentSpaceSurfaceNormal = FVector::ForwardVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FPatchProximityResult representativeResult;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    TArray<FPatchProximityFrame> frames;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FTargetSemanticEventEvaluationSettings
{
    GENERATED_BODY()

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Target Semantic Evaluation",
        meta = (ClampMin = "0.0", Units = "cm"))
    double normalTolerance = 0.25;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Target Semantic Evaluation",
        meta = (ClampMin = "0.0", Units = "cm"))
    double tangentialTolerance = 0.25;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FTargetSemanticEventEvaluation
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    bool isValid = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    bool needsCorrection = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    FName pointId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    FSemanticInteractionPoint interactionPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    FVector componentSpaceCurrentPoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    FVector componentSpaceGoal = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    FVector componentSpaceError = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation", meta = (Units = "cm"))
    double distanceError = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation", meta = (Units = "cm"))
    double normalError = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation")
    FVector componentSpaceTangentialError = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Semantic Evaluation", meta = (Units = "cm"))
    double tangentialError = 0.0;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FMappedSemanticPatchEvent
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    bool isValid = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FSourceSemanticPatchEvent sourceEvent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FPatchProximityTarget target;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FTargetSemanticEventEvaluation targetEvaluation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FVector worldSpacePatchPoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FVector worldSpaceSurfaceNormal = FVector::ForwardVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FVector worldSpaceInteractionGoal = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapped Semantic Event")
    FVector worldSpaceCurrentInteractionPoint = FVector::ZeroVector;
};

UENUM(BlueprintType)
enum class EContextOptimizingHand : uint8
{
    Left,
    Right
};

UENUM(BlueprintType)
enum class EContextOptimizingHandRegion : uint8
{
    Hand,
    Thumb,
    Index,
    Middle,
    Ring,
    Pinky
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FSourceProximitySample
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 frame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    EContextOptimizingHand hand = EContextOptimizingHand::Left;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    EContextOptimizingHandRegion handRegion = EContextOptimizingHandRegion::Hand;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 meshVertexIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event", meta = (DisplayName = "Source semantic face patch"))
    FName sourceSemanticFacePatch;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector handPosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector facePosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FVector faceNormal = FVector::ForwardVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event", meta = (Units = "cm"))
    double signedNormalOffsetCm = 0.0;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FSourceSemanticEvent
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 startFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 closestApproachFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    int32 endFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    EContextOptimizingHand hand = EContextOptimizingHand::Left;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    FSourceProximitySample primarySample;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Source Semantic Event")
    TArray<FSourceProximitySample> secondarySamples;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FTargetPenetrationSample
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    int32 frame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    EContextOptimizingHand hand = EContextOptimizingHand::Left;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    EContextOptimizingHandRegion handRegion = EContextOptimizingHandRegion::Hand;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    int32 meshVertexIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event", meta = (DisplayName = "Face patch penetrating on target"))
    FName facePatchPenetratingOnTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    FVector handPosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    FVector facePosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    FVector faceNormal = FVector::ForwardVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event", meta = (Units = "cm"))
    double penetrationDepthCm = 0.0;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FTargetSafetyEvent
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    int32 startFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    int32 worstPenetrationFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    int32 endFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    EContextOptimizingHand hand = EContextOptimizingHand::Left;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    bool overlapsSourceSemanticEvent = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Safety Event")
    FTargetPenetrationSample worstSample;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FSourceProximityDetectionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source Proximity", meta = (ClampMin = "0", Units = "cm"))
    double enterDistanceCm = 2.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source Proximity", meta = (ClampMin = "0", Units = "cm"))
    double exitDistanceCm = 3.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source Proximity", meta = (ClampMin = "0"))
    int32 eventGapToleranceFrames = 1;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZING_API FTargetSafetyDetectionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Safety", meta = (ClampMin = "0", Units = "cm"))
    double clearanceCm = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Safety", meta = (ClampMin = "0", Units = "cm"))
    double enterPenetrationThresholdCm = 0.01;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Safety", meta = (ClampMin = "0", Units = "cm"))
    double exitPenetrationThresholdCm = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Safety", meta = (ClampMin = "0"))
    int32 eventGapToleranceFrames = 1;
};

