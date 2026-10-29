#include "ContextOptimizationPipelineEditorLibrary.h"

#include "Animation/AnimSequence.h"
#include "BoneCorrectionEditorLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContextOptimizingEventEditorLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "SemanticEventTimingEditorLibrary.h"
#include "SurfacePatchesComponent.h"

namespace
{
int32 frameAtTime(
    const UAnimSequenceBase& animation,
    const double timeSeconds)
{
    const FFrameRate frameRate = animation.GetSamplingFrameRate();
    return FMath::Clamp(
        FMath::RoundToInt(timeSeconds * frameRate.AsDecimal()),
        0,
        animation.GetNumberOfSampledKeys() - 1);
}

double timeAtFrame(
    const UAnimSequenceBase& animation,
    const int32 frame)
{
    return animation.GetSamplingFrameRate().AsSeconds(frame);
}

void addIdentityKey(
    UAdditiveBoneCorrectionLayer& layer,
    const int32 frame)
{
    FAdditiveBoneCorrectionKey key;
    key.frame = frame;
    layer.addOrUpdateKey(key);
}
}

FContextOptimizationPipelineResult
UContextOptimizationPipelineEditorLibrary::bakeSemanticEventCorrections(
    const UAnimSequenceBase* sourceAnimation,
    const AActor* sourceActor,
    UAnimSequence* initialTargetAnimation,
    const AActor* targetActor,
    const FContextOptimizationPipelineSettings& settings)
{
    FContextOptimizationPipelineResult result;
    if (sourceAnimation == nullptr ||
        sourceActor == nullptr ||
        initialTargetAnimation == nullptr ||
        targetActor == nullptr)
    {
        result.errorMessage = TEXT(
            "Source animation, source actor, target animation, and target actor are required.");
        return result;
    }
    if (settings.sourceMovementBone.IsNone())
    {
        result.errorMessage = TEXT("A source movement bone is required.");
        return result;
    }
    if (settings.targetIkChain.bones.IsEmpty())
    {
        result.errorMessage = TEXT("A target IK chain is required.");
        return result;
    }
    if (settings.outputAssetName.IsEmpty())
    {
        result.errorMessage = TEXT("An output asset name is required.");
        return result;
    }

    const USurfacePatchesComponent* targetSurfacePatches =
        targetActor->FindComponentByClass<USurfacePatchesComponent>();
    const USkeletalMeshComponent* targetMeshComponent =
        targetSurfacePatches != nullptr
            ? targetSurfacePatches->getTargetMeshComponent()
            : nullptr;
    const USkeletalMesh* targetSkeletalMesh =
        targetMeshComponent != nullptr
            ? targetMeshComponent->GetSkeletalMeshAsset()
            : nullptr;
    if (targetSkeletalMesh == nullptr)
    {
        result.errorMessage = TEXT(
            "The target actor needs a SurfacePatches component with a target skeletal mesh.");
        return result;
    }

    TArray<FMappedSemanticPatchEvent> mappedEvents =
        UContextOptimizingEventEditorLibrary::extractAndMapSemanticPatchEvents(
            sourceAnimation,
            sourceActor,
            initialTargetAnimation,
            targetActor,
            settings.proximitySettings,
            settings.targetEvaluationSettings,
            settings.maximumFrameDistance,
            settings.targetNormalOffset);
    mappedEvents.Sort(
        [](const FMappedSemanticPatchEvent& left,
           const FMappedSemanticPatchEvent& right)
        {
            return left.sourceEvent.representativeFrame <
                right.sourceEvent.representativeFrame;
        });
    if (mappedEvents.IsEmpty())
    {
        result.errorMessage = TEXT("No mapped source semantic events were found.");
        return result;
    }

    const FFrameRate sourceFrameRate = sourceAnimation->GetSamplingFrameRate();
    const FFrameRate targetFrameRate = initialTargetAnimation->GetSamplingFrameRate();
    if (!sourceFrameRate.IsValid() || !targetFrameRate.IsValid())
    {
        result.errorMessage = TEXT("Source and target animations need valid frame rates.");
        return result;
    }

    TArray<int32> preparedCorrectionIndices;
    result.eventCorrections.Reserve(mappedEvents.Num());
    for (const FMappedSemanticPatchEvent& mappedEvent : mappedEvents)
    {
        FContextOptimizationEventCorrection& eventCorrection =
            result.eventCorrections.AddDefaulted_GetRef();
        eventCorrection.mappedEvent = mappedEvent;
        if (!mappedEvent.isValid)
        {
            eventCorrection.errorMessage = TEXT("The mapped semantic event is invalid.");
            continue;
        }
        if (settings.onlyCorrectWhenNeeded &&
            !mappedEvent.targetEvaluation.needsCorrection)
        {
            eventCorrection.errorMessage = TEXT("The target event is already within tolerance.");
            continue;
        }

        if (!FSemanticZeroCalculator::calculateSemanticZeroFrame(
                sourceAnimation,
                settings.sourceMovementBone,
                mappedEvent.sourceEvent.representativeFrame,
                mappedEvent.sourceEvent.startFrame,
                ESemanticZeroType::Approach,
                eventCorrection.sourceApproachStartFrame,
                eventCorrection.errorMessage))
        {
            continue;
        }
        if (!FSemanticZeroCalculator::calculateSemanticZeroFrame(
                sourceAnimation,
                settings.sourceMovementBone,
                mappedEvent.sourceEvent.representativeFrame,
                mappedEvent.sourceEvent.endFrame,
                ESemanticZeroType::Recovery,
                eventCorrection.sourceRecoveryEndFrame,
                eventCorrection.errorMessage))
        {
            continue;
        }

        eventCorrection.targetApproachStartFrame = frameAtTime(
            *initialTargetAnimation,
            timeAtFrame(*sourceAnimation, eventCorrection.sourceApproachStartFrame));
        eventCorrection.targetEventStartFrame = frameAtTime(
            *initialTargetAnimation,
            timeAtFrame(*sourceAnimation, mappedEvent.sourceEvent.startFrame));
        eventCorrection.targetRepresentativeFrame = frameAtTime(
            *initialTargetAnimation,
            mappedEvent.sourceEvent.representativeTimeSeconds);
        eventCorrection.targetEventEndFrame = frameAtTime(
            *initialTargetAnimation,
            timeAtFrame(*sourceAnimation, mappedEvent.sourceEvent.endFrame));
        eventCorrection.targetRecoveryEndFrame = frameAtTime(
            *initialTargetAnimation,
            timeAtFrame(*sourceAnimation, eventCorrection.sourceRecoveryEndFrame));

        eventCorrection.targetApproachStartFrame = FMath::Min(
            eventCorrection.targetApproachStartFrame,
            eventCorrection.targetRepresentativeFrame - 1);
        eventCorrection.targetEventStartFrame = FMath::Clamp(
            eventCorrection.targetEventStartFrame,
            eventCorrection.targetApproachStartFrame + 1,
            eventCorrection.targetRepresentativeFrame);
        eventCorrection.targetEventEndFrame = FMath::Max(
            eventCorrection.targetEventEndFrame,
            eventCorrection.targetRepresentativeFrame);
        eventCorrection.targetRecoveryEndFrame = FMath::Max(
            eventCorrection.targetRecoveryEndFrame,
            eventCorrection.targetEventEndFrame + 1);
        if (eventCorrection.targetApproachStartFrame < 0 ||
            eventCorrection.targetRecoveryEndFrame >=
                initialTargetAnimation->GetNumberOfSampledKeys())
        {
            eventCorrection.errorMessage = TEXT(
                "The mapped target animation has no room for approach or recovery keys.");
            continue;
        }

        const FSemanticInteractionPoint& interactionPoint =
            mappedEvent.targetEvaluation.interactionPoint;
        eventCorrection.solveResult =
            UBoneCorrectionEditorLibrary::solvePositionIkAtFrame(
                initialTargetAnimation,
                targetSkeletalMesh,
                eventCorrection.targetRepresentativeFrame,
                interactionPoint.parentBone,
                interactionPoint.boneLocalPosition,
                mappedEvent.target.componentSpaceInteractionGoal,
                settings.targetIkChain,
                settings.ikSettings);
        if (!eventCorrection.solveResult.isValid)
        {
            eventCorrection.errorMessage =
                eventCorrection.solveResult.errorMessage;
            continue;
        }
        preparedCorrectionIndices.Add(result.eventCorrections.Num() - 1);
    }

    if (preparedCorrectionIndices.IsEmpty())
    {
        result.errorMessage = TEXT("No semantic event produced a complete correction.");
        return result;
    }

    FString layerError;
    const FString layerAssetName =
        FString(TEXT("Layer_")) + settings.outputAssetName;
    result.correctionLayer =
        UBoneCorrectionEditorLibrary::createCorrectionLayerAsset(
            initialTargetAnimation,
            settings.outputPackagePath,
            layerAssetName,
            layerError);
    if (result.correctionLayer == nullptr)
    {
        result.errorMessage = layerError;
        return result;
    }

    for (int32 preparedIndex = 0;
         preparedIndex < preparedCorrectionIndices.Num();
         ++preparedIndex)
    {
        FContextOptimizationEventCorrection& current =
            result.eventCorrections[preparedCorrectionIndices[preparedIndex]];
        const FContextOptimizationEventCorrection* previous =
            preparedIndex > 0
                ? &result.eventCorrections[
                    preparedCorrectionIndices[preparedIndex - 1]]
                : nullptr;
        const FContextOptimizationEventCorrection* next =
            preparedIndex + 1 < preparedCorrectionIndices.Num()
                ? &result.eventCorrections[
                    preparedCorrectionIndices[preparedIndex + 1]]
                : nullptr;

        if (previous == nullptr ||
            previous->targetRecoveryEndFrame <
                current.targetApproachStartFrame)
        {
            addIdentityKey(
                *result.correctionLayer,
                current.targetApproachStartFrame);
        }

        if (current.targetEventStartFrame <
            current.targetRepresentativeFrame)
        {
            FAdditiveBoneCorrectionKey eventStartKey =
                current.solveResult.correctionKey;
            eventStartKey.frame = current.targetEventStartFrame;
            result.correctionLayer->addOrUpdateKey(eventStartKey);
        }
        result.correctionLayer->addOrUpdateKey(current.solveResult.correctionKey);
        if (current.targetEventEndFrame >
            current.targetRepresentativeFrame)
        {
            FAdditiveBoneCorrectionKey holdEndKey =
                current.solveResult.correctionKey;
            holdEndKey.frame = current.targetEventEndFrame;
            result.correctionLayer->addOrUpdateKey(holdEndKey);
        }

        if (next == nullptr ||
            current.targetRecoveryEndFrame <
                next->targetApproachStartFrame)
        {
            addIdentityKey(
                *result.correctionLayer,
                current.targetRecoveryEndFrame);
        }
        current.applied = true;
        current.errorMessage.Reset();
    }

    const FBoneCorrectionBakeResult bakeResult =
        UBoneCorrectionEditorLibrary::bakeCorrectionLayer(
            result.correctionLayer,
            targetSkeletalMesh,
            settings.outputPackagePath,
            settings.outputAssetName);
    if (!bakeResult.success)
    {
        result.errorMessage = bakeResult.errorMessage;
        return result;
    }

    result.animation = bakeResult.animation;
    result.success = true;
    return result;
}
