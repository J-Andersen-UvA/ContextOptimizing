#include "PatchProximityEventLibrary.h"

#include "Algo/Sort.h"
#include "SemanticPatchProximityLibrary.h"

namespace
{
struct FPatchSelectionScore
{
    FName patchId;
    int32 frameVotes = 0;
    int32 sampleCount = 0;
    double normalizedCenterDistanceSum = 0.0;
    double closestDistance = TNumericLimits<double>::Max();
};

bool isPreferredFrameResult(
    const FPatchProximityResult& candidate,
    const FPatchProximityResult& current)
{
    const double candidateCenterDistance =
        candidate.normalizedPatchLocation.SizeSquared();
    const double currentCenterDistance =
        current.normalizedPatchLocation.SizeSquared();
    if (!FMath::IsNearlyEqual(candidateCenterDistance, currentCenterDistance))
    {
        return candidateCenterDistance < currentCenterDistance;
    }
    if (!FMath::IsNearlyEqual(candidate.distance, current.distance))
    {
        return candidate.distance < current.distance;
    }
    return candidate.patchId.LexicalLess(current.patchId);
}

bool isPreferredPatchScore(
    const FPatchSelectionScore& candidate,
    const FPatchSelectionScore& current)
{
    if (candidate.frameVotes != current.frameVotes)
    {
        return candidate.frameVotes > current.frameVotes;
    }

    const double candidateAverageCenterDistance =
        candidate.normalizedCenterDistanceSum /
        FMath::Max(1, candidate.sampleCount);
    const double currentAverageCenterDistance =
        current.normalizedCenterDistanceSum /
        FMath::Max(1, current.sampleCount);
    if (!FMath::IsNearlyEqual(
            candidateAverageCenterDistance,
            currentAverageCenterDistance))
    {
        return candidateAverageCenterDistance < currentAverageCenterDistance;
    }
    if (!FMath::IsNearlyEqual(candidate.closestDistance, current.closestDistance))
    {
        return candidate.closestDistance < current.closestDistance;
    }
    return candidate.patchId.LexicalLess(current.patchId);
}
}

TArray<FPatchProximityFrameNeighborGroup>
UPatchProximityEventLibrary::getPatchProximityFrameNeighbors(
    const FPatchProximityAnimationData& animationData,
    const int32 maximumFrameDistance)
{
    TArray<FPatchProximityFrameNeighborGroup> groups;
    if (!animationData.isValid)
    {
        return groups;
    }

    TArray<const FPatchProximityFrame*> populatedFrames;
    populatedFrames.Reserve(animationData.frames.Num());
    for (const FPatchProximityFrame& frame : animationData.frames)
    {
        if (frame.frameIndex != INDEX_NONE && !frame.results.IsEmpty())
        {
            populatedFrames.Add(&frame);
        }
    }

    Algo::SortBy(
        populatedFrames,
        [](const FPatchProximityFrame* frame)
        {
            return frame->frameIndex;
        });

    const int32 clampedFrameDistance = FMath::Max(1, maximumFrameDistance);
    for (const FPatchProximityFrame* frame : populatedFrames)
    {
        const bool startsNewGroup =
            groups.IsEmpty() ||
            frame->frameIndex - groups.Last().endFrame > clampedFrameDistance;
        if (startsNewGroup)
        {
            FPatchProximityFrameNeighborGroup& group = groups.AddDefaulted_GetRef();
            group.startFrame = frame->frameIndex;
        }

        FPatchProximityFrameNeighborGroup& group = groups.Last();
        group.endFrame = frame->frameIndex;
        group.frames.Add(*frame);
    }

    return groups;
}

FSourceSemanticPatchEvent
UPatchProximityEventLibrary::buildSourceSemanticPatchEvent(
    const FPatchProximityPointNeighborGroup& pointNeighbors)
{
    FSourceSemanticPatchEvent semanticEvent;
    semanticEvent.pointId = pointNeighbors.pointId;
    semanticEvent.interactionGroupId = pointNeighbors.interactionGroupId;
    semanticEvent.startFrame = pointNeighbors.startFrame;
    semanticEvent.endFrame = pointNeighbors.endFrame;

    TArray<FPatchSelectionScore> patchScores;
    TMap<FName, int32> scoreIndexByPatchId;
    for (const FPatchProximityFrame& frame : pointNeighbors.frames)
    {
        const FPatchProximityResult* preferredFrameResult = nullptr;
        for (const FPatchProximityResult& result : frame.results)
        {
            if (result.pointId != pointNeighbors.pointId)
            {
                continue;
            }

            int32* scoreIndex = scoreIndexByPatchId.Find(result.patchId);
            if (scoreIndex == nullptr)
            {
                const int32 newScoreIndex = patchScores.AddDefaulted();
                patchScores[newScoreIndex].patchId = result.patchId;
                scoreIndexByPatchId.Add(result.patchId, newScoreIndex);
                scoreIndex = scoreIndexByPatchId.Find(result.patchId);
            }

            FPatchSelectionScore& score = patchScores[*scoreIndex];
            ++score.sampleCount;
            score.normalizedCenterDistanceSum +=
                result.normalizedPatchLocation.SizeSquared();
            score.closestDistance = FMath::Min(
                score.closestDistance,
                result.distance);

            if (preferredFrameResult == nullptr ||
                isPreferredFrameResult(result, *preferredFrameResult))
            {
                preferredFrameResult = &result;
            }
        }

        if (preferredFrameResult != nullptr)
        {
            ++patchScores[
                scoreIndexByPatchId.FindChecked(preferredFrameResult->patchId)]
                  .frameVotes;
        }
    }

    if (patchScores.IsEmpty())
    {
        return semanticEvent;
    }

    const FPatchSelectionScore* selectedScore = &patchScores[0];
    for (int32 scoreIndex = 1; scoreIndex < patchScores.Num(); ++scoreIndex)
    {
        if (isPreferredPatchScore(patchScores[scoreIndex], *selectedScore))
        {
            selectedScore = &patchScores[scoreIndex];
        }
    }

    const FPatchProximityResult* representativeResult = nullptr;
    int32 representativeFrame = INDEX_NONE;
    for (const FPatchProximityFrame& frame : pointNeighbors.frames)
    {
        FPatchProximityFrame selectedFrame;
        selectedFrame.frameIndex = frame.frameIndex;
        selectedFrame.timeSeconds = frame.timeSeconds;

        for (const FPatchProximityResult& result : frame.results)
        {
            if (result.pointId != pointNeighbors.pointId ||
                result.patchId != selectedScore->patchId)
            {
                continue;
            }

            selectedFrame.results.Add(result);
            if (representativeResult == nullptr ||
                result.distance < representativeResult->distance)
            {
                representativeResult = &result;
                representativeFrame = frame.frameIndex;
            }
        }

        if (!selectedFrame.results.IsEmpty())
        {
            semanticEvent.frames.Add(MoveTemp(selectedFrame));
        }
    }

    if (representativeResult == nullptr)
    {
        return semanticEvent;
    }

    semanticEvent.isValid = true;
    semanticEvent.patchId = selectedScore->patchId;
    semanticEvent.representativeFrame = representativeFrame;
    for (const FPatchProximityFrame& frame : pointNeighbors.frames)
    {
        if (frame.frameIndex == representativeFrame)
        {
            semanticEvent.representativeTimeSeconds = frame.timeSeconds;
            break;
        }
    }
    semanticEvent.normalizedPatchLocation =
        representativeResult->normalizedPatchLocation;
    semanticEvent.signedNormalOffset = representativeResult->signedDistance;
    semanticEvent.componentSpaceInteractionPoint =
        representativeResult->componentSpaceInteractionPoint;
    semanticEvent.componentSpacePatchPoint =
        representativeResult->componentSpacePatchPoint;
    semanticEvent.componentSpaceSurfaceNormal =
        representativeResult->componentSpaceSurfaceNormal;
    semanticEvent.representativeResult = *representativeResult;
    return semanticEvent;
}

FPatchProximityTarget
UPatchProximityEventLibrary::mapSourceSemanticPatchEventToTarget(
    const FSourceSemanticPatchEvent& sourceEvent,
    const FSemanticPose& targetPose,
    const TArray<FSurfacePatchAnchor>& targetPatches,
    const double targetNormalOffset)
{
    if (!sourceEvent.isValid)
    {
        return FPatchProximityTarget();
    }

    return USemanticPatchProximityLibrary::mapPatchProximityToTarget(
        sourceEvent.representativeResult,
        targetPose,
        targetPatches,
        targetNormalOffset);
}

FTargetSemanticEventEvaluation
UPatchProximityEventLibrary::evaluateTargetSemanticPatchEvent(
    const FSourceSemanticPatchEvent& sourceEvent,
    const FPatchProximityTarget& target,
    const FSemanticPose& targetPose,
    const TArray<FSemanticInteractionPoint>& targetInteractionPoints,
    const FTargetSemanticEventEvaluationSettings& settings)
{
    FTargetSemanticEventEvaluation evaluation;
    evaluation.pointId = sourceEvent.pointId;
    if (!sourceEvent.isValid || !target.isValid)
    {
        return evaluation;
    }

    const FSemanticInteractionPoint* matchingPoint =
        targetInteractionPoints.FindByPredicate(
            [&sourceEvent](const FSemanticInteractionPoint& point)
            {
                return point.enabled && point.pointId == sourceEvent.pointId;
            });
    if (matchingPoint == nullptr)
    {
        return evaluation;
    }

    FTransform parentBoneTransform;
    if (!targetPose.getBoneTransform(
            matchingPoint->parentBone,
            parentBoneTransform))
    {
        return evaluation;
    }

    const FVector surfaceNormal =
        target.componentSpaceSurfaceNormal.GetSafeNormal();
    if (surfaceNormal.IsNearlyZero())
    {
        return evaluation;
    }

    evaluation.isValid = true;
    evaluation.interactionPoint = *matchingPoint;
    evaluation.componentSpaceCurrentPoint =
        parentBoneTransform.TransformPosition(
            matchingPoint->boneLocalPosition);
    evaluation.componentSpaceGoal = target.componentSpaceInteractionGoal;
    evaluation.componentSpaceError =
        evaluation.componentSpaceGoal -
        evaluation.componentSpaceCurrentPoint;
    evaluation.distanceError = evaluation.componentSpaceError.Length();
    evaluation.normalError = FVector::DotProduct(
        evaluation.componentSpaceError,
        surfaceNormal);
    evaluation.componentSpaceTangentialError =
        evaluation.componentSpaceError -
        surfaceNormal * evaluation.normalError;
    evaluation.tangentialError =
        evaluation.componentSpaceTangentialError.Length();
    evaluation.needsCorrection =
        FMath::Abs(evaluation.normalError) >
            FMath::Max(0.0, settings.normalTolerance) ||
        evaluation.tangentialError >
            FMath::Max(0.0, settings.tangentialTolerance);
    return evaluation;
}

TArray<FPatchProximityPointNeighborGroup>
UPatchProximityEventLibrary::getPatchProximityPointNeighbors(
    const FPatchProximityFrameNeighborGroup& frameNeighbors)
{
    TArray<FPatchProximityPointNeighborGroup> groups;
    TMap<FName, int32> groupIndexByPointId;

    for (const FPatchProximityFrame& frame : frameNeighbors.frames)
    {
        TMap<FName, int32> frameIndexByPointId;
        for (const FPatchProximityResult& result : frame.results)
        {
            int32* existingGroupIndex = groupIndexByPointId.Find(result.pointId);
            if (existingGroupIndex == nullptr)
            {
                const int32 newGroupIndex = groups.AddDefaulted();
                groupIndexByPointId.Add(result.pointId, newGroupIndex);

                FPatchProximityPointNeighborGroup& newGroup = groups[newGroupIndex];
                newGroup.pointId = result.pointId;
                newGroup.interactionGroupId = result.interactionGroupId;
                newGroup.startFrame = frame.frameIndex;
                existingGroupIndex = groupIndexByPointId.Find(result.pointId);
            }

            FPatchProximityPointNeighborGroup& group = groups[*existingGroupIndex];
            group.endFrame = frame.frameIndex;

            int32* existingFrameIndex = frameIndexByPointId.Find(result.pointId);
            if (existingFrameIndex == nullptr)
            {
                FPatchProximityFrame& filteredFrame = group.frames.AddDefaulted_GetRef();
                filteredFrame.frameIndex = frame.frameIndex;
                filteredFrame.timeSeconds = frame.timeSeconds;
                frameIndexByPointId.Add(result.pointId, group.frames.Num() - 1);
                existingFrameIndex = frameIndexByPointId.Find(result.pointId);
            }

            group.frames[*existingFrameIndex].results.Add(result);
        }
    }

    return groups;
}
