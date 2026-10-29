#include "ContextOptimizingEventEditorLibrary.h"

#include "AnimPose.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "PatchProximityEventLibrary.h"
#include "SemanticPatchProximityEditorLibrary.h"
#include "SemanticPatchProximityLibrary.h"
#include "SurfacePatchesComponent.h"

namespace
{
FAnimPoseEvaluationOptions makeEvaluationOptions(
    const USurfacePatchesComponent* surfacePatches)
{
    FAnimPoseEvaluationOptions options;
    options.EvaluationType = EAnimDataEvalType::Raw;
    options.bShouldRetarget = true;
    options.bEvaluateCurves = false;

    if (surfacePatches != nullptr)
    {
        const USkeletalMeshComponent* skeletalMesh =
            surfacePatches->getTargetMeshComponent();
        if (skeletalMesh != nullptr)
        {
            options.OptionalSkeletalMesh = skeletalMesh->GetSkeletalMeshAsset();
        }
    }
    return options;
}
}

TArray<FMappedSemanticPatchEvent>
UContextOptimizingEventEditorLibrary::extractAndMapSemanticPatchEvents(
    const UAnimSequenceBase* sourceAnimation,
    const AActor* sourceActor,
    const UAnimSequenceBase* targetAnimation,
    const AActor* targetActor,
    const FPatchProximitySettings& proximitySettings,
    const FTargetSemanticEventEvaluationSettings& targetEvaluationSettings,
    const int32 maximumFrameDistance,
    const double targetNormalOffset)
{
    TArray<FMappedSemanticPatchEvent> mappedEvents;
    if (sourceAnimation == nullptr ||
        sourceActor == nullptr ||
        targetAnimation == nullptr ||
        targetActor == nullptr)
    {
        return mappedEvents;
    }

    const USurfacePatchesComponent* sourceSurfacePatches =
        sourceActor->FindComponentByClass<USurfacePatchesComponent>();
    const USurfacePatchesComponent* targetSurfacePatches =
        targetActor->FindComponentByClass<USurfacePatchesComponent>();
    if (sourceSurfacePatches == nullptr || targetSurfacePatches == nullptr)
    {
        return mappedEvents;
    }

    const TArray<FSurfacePatchAnchor> sourcePatches =
        sourceSurfacePatches->getEnabledPatches();
    const TArray<FSurfacePatchAnchor> targetPatches =
        targetSurfacePatches->getEnabledPatches();
    const TArray<FSemanticInteractionPoint> interactionPoints =
        USemanticPatchProximityLibrary::getInteractionPoints(sourceActor);
    const TArray<FSemanticInteractionPoint> targetInteractionPoints =
        USemanticPatchProximityLibrary::getInteractionPoints(targetActor);
    if (sourcePatches.IsEmpty() ||
        targetPatches.IsEmpty() ||
        interactionPoints.IsEmpty() ||
        targetInteractionPoints.IsEmpty())
    {
        return mappedEvents;
    }

    const FPatchProximityAnimationData animationData =
        USemanticPatchProximityEditorLibrary::extractPatchProximityFromAnimation(
            sourceAnimation,
            makeEvaluationOptions(sourceSurfacePatches),
            sourcePatches,
            interactionPoints,
            proximitySettings);
    const TArray<FPatchProximityFrameNeighborGroup> frameGroups =
        UPatchProximityEventLibrary::getPatchProximityFrameNeighbors(
            animationData,
            maximumFrameDistance);

    const USkeletalMeshComponent* targetMesh =
        targetSurfacePatches->getTargetMeshComponent();
    if (targetMesh == nullptr)
    {
        return mappedEvents;
    }

    const FTransform targetComponentTransform =
        targetMesh->GetComponentTransform();
    const FAnimPoseEvaluationOptions targetEvaluationOptions =
        makeEvaluationOptions(targetSurfacePatches);

    for (const FPatchProximityFrameNeighborGroup& frameGroup : frameGroups)
    {
        const TArray<FPatchProximityPointNeighborGroup> pointGroups =
            UPatchProximityEventLibrary::getPatchProximityPointNeighbors(
                frameGroup);
        for (const FPatchProximityPointNeighborGroup& pointGroup : pointGroups)
        {
            FMappedSemanticPatchEvent& mappedEvent =
                mappedEvents.AddDefaulted_GetRef();
            mappedEvent.sourceEvent =
                UPatchProximityEventLibrary::buildSourceSemanticPatchEvent(
                    pointGroup);
            if (!mappedEvent.sourceEvent.isValid)
            {
                continue;
            }

            FAnimPose targetAnimPose;
            UAnimPoseExtensions::GetAnimPoseAtTime(
                targetAnimation,
                FMath::Clamp(
                    mappedEvent.sourceEvent.representativeTimeSeconds,
                    0.0,
                    targetAnimation->GetPlayLength()),
                targetEvaluationOptions,
                targetAnimPose);
            if (!UAnimPoseExtensions::IsValid(targetAnimPose))
            {
                continue;
            }

            const FSemanticPose targetPose =
                USemanticPatchProximityEditorLibrary::makeSemanticPoseFromAnimPose(
                    targetAnimPose);
            mappedEvent.target =
                UPatchProximityEventLibrary::mapSourceSemanticPatchEventToTarget(
                    mappedEvent.sourceEvent,
                    targetPose,
                    targetPatches,
                    targetNormalOffset);
            if (!mappedEvent.target.isValid)
            {
                continue;
            }

            mappedEvent.targetEvaluation =
                UPatchProximityEventLibrary::evaluateTargetSemanticPatchEvent(
                    mappedEvent.sourceEvent,
                    mappedEvent.target,
                    targetPose,
                    targetInteractionPoints,
                    targetEvaluationSettings);
            if (!mappedEvent.targetEvaluation.isValid)
            {
                continue;
            }

            mappedEvent.isValid = true;
            mappedEvent.worldSpacePatchPoint =
                targetComponentTransform.TransformPosition(
                    mappedEvent.target.componentSpacePatchPoint);
            mappedEvent.worldSpaceSurfaceNormal =
                targetComponentTransform.TransformVectorNoScale(
                    mappedEvent.target.componentSpaceSurfaceNormal)
                  .GetSafeNormal();
            mappedEvent.worldSpaceInteractionGoal =
                targetComponentTransform.TransformPosition(
                    mappedEvent.target.componentSpaceInteractionGoal);
            mappedEvent.worldSpaceCurrentInteractionPoint =
                targetComponentTransform.TransformPosition(
                    mappedEvent.targetEvaluation.componentSpaceCurrentPoint);
        }
    }

    return mappedEvents;
}
