#include "SemanticEventTimingEditorLibrary.h"

#include "AnimPose.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"

namespace
{
bool sampleBonePositions(
    const UAnimSequenceBase* animation,
    FName boneName,
    TArray<FVector>& positions,
    FString& errorMessage)
{
    positions.Reset();
    errorMessage.Reset();
    if (animation == nullptr || animation->GetSkeleton() == nullptr)
    {
        errorMessage = TEXT("Animation with a valid skeleton is required.");
        return false;
    }
    if (boneName.IsNone() ||
        animation->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(boneName) == INDEX_NONE)
    {
        errorMessage = TEXT("Bone does not exist on the animation skeleton.");
        return false;
    }

    const FFrameRate frameRate = animation->GetSamplingFrameRate();
    const int32 frameCount = animation->GetNumberOfSampledKeys();
    if (!frameRate.IsValid() || frameCount < 2)
    {
        errorMessage = TEXT("Animation needs a valid frame rate and at least two frames.");
        return false;
    }

    positions.Reserve(frameCount);
    FAnimPoseEvaluationOptions evaluationOptions;
    evaluationOptions.bEvaluateCurves = false;
    constexpr int32 poseBatchSize = 256;
    for (int32 batchStart = 0; batchStart < frameCount; batchStart += poseBatchSize)
    {
        const int32 batchEnd = FMath::Min(batchStart + poseBatchSize, frameCount);
        TArray<double> sampleTimes;
        sampleTimes.Reserve(batchEnd - batchStart);
        for (int32 frameIndex = batchStart; frameIndex < batchEnd; ++frameIndex)
        {
            sampleTimes.Add(frameRate.AsSeconds(frameIndex));
        }

        TArray<FAnimPose> poses;
        UAnimPoseExtensions::GetAnimPoseAtTimeIntervals(
            animation,
            sampleTimes,
            evaluationOptions,
            poses);
        if (poses.Num() != sampleTimes.Num())
        {
            positions.Reset();
            errorMessage = TEXT("Unreal did not return every requested animation pose.");
            return false;
        }
        for (const FAnimPose& pose : poses)
        {
            if (!UAnimPoseExtensions::IsValid(pose))
            {
                positions.Reset();
                errorMessage = TEXT("Unreal returned an invalid animation pose.");
                return false;
            }
            positions.Add(UAnimPoseExtensions::GetBonePose(
                pose,
                boneName,
                EAnimPoseSpaces::World).GetLocation());
        }
    }
    return true;
}

}

bool FSemanticZeroCalculator::calculateSemanticZeroFrame(
    const UAnimSequenceBase* animation,
    FName boneName,
    int32 representativeFrame,
    int32 eventBoundaryFrame,
    ESemanticZeroType zeroType,
    int32& zeroFrame,
    FString& errorMessage)
{
    zeroFrame = INDEX_NONE;

    TArray<FVector> positions;
    if (!sampleBonePositions(
            animation,
            boneName,
            positions,
            errorMessage))
    {
        return false;
    }
    if (!positions.IsValidIndex(representativeFrame) ||
        !positions.IsValidIndex(eventBoundaryFrame))
    {
        errorMessage = TEXT("Representative and event-boundary frames are invalid.");
        return false;
    }
    if (zeroType == ESemanticZeroType::Approach && representativeFrame < 3)
    {
        errorMessage = FString::Printf(
            TEXT("Representative frame must be between 3 and %d."),
            positions.Num() - 1);
        return false;
    }
    if (zeroType == ESemanticZeroType::Recovery &&
        eventBoundaryFrame < representativeFrame)
    {
        errorMessage = TEXT("Recovery event boundary cannot precede the representative frame.");
        return false;
    }

    const FVector goalLocation = positions[representativeFrame];
    TArray<double> distances;
    distances.Reserve(positions.Num());
    double maximumDistance = 0.0;
    for (int32 frameIndex = 0; frameIndex < positions.Num(); ++frameIndex)
    {
        const double distance = FVector::Distance(
            positions[frameIndex],
            goalLocation);
        distances.Add(distance);
        const bool isInSearchRange =
            zeroType == ESemanticZeroType::Approach
                ? frameIndex <= representativeFrame
                : frameIndex >= eventBoundaryFrame;
        if (isInSearchRange)
        {
            maximumDistance = FMath::Max(maximumDistance, distance);
        }
    }

    TArray<double> smoothedDistances;
    smoothedDistances.SetNumUninitialized(distances.Num());
    constexpr int32 smoothingRadius = 1;
    for (int32 frameIndex = 0; frameIndex < distances.Num(); ++frameIndex)
    {
        double sum = 0.0;
        int32 sampleCount = 0;
        for (int32 offset = -smoothingRadius;
             offset <= smoothingRadius;
             ++offset)
        {
            const int32 sampleIndex = frameIndex + offset;
            if (distances.IsValidIndex(sampleIndex))
            {
                sum += distances[sampleIndex];
                ++sampleCount;
            }
        }
        smoothedDistances[frameIndex] = sum / sampleCount;
    }

    constexpr int32 progressWindowFrames = 3;
    constexpr int32 toleratedNonApproachWindows = 2;
    const double minimumProgressCm = FMath::Max(
        0.02,
        maximumDistance * 0.002);

    if (zeroType == ESemanticZeroType::Approach)
    {
        bool foundApproach = false;
        int32 nonApproachWindows = 0;
        int32 earliestApproachFrame = INDEX_NONE;
        for (int32 frameIndex = representativeFrame;
             frameIndex >= progressWindowFrames;
             --frameIndex)
        {
            const int32 earlierFrame = frameIndex - progressWindowFrames;
            const double progressTowardGoal =
                smoothedDistances[earlierFrame] -
                smoothedDistances[frameIndex];
            if (progressTowardGoal > minimumProgressCm)
            {
                foundApproach = true;
                nonApproachWindows = 0;
                earliestApproachFrame = earlierFrame;
            }
            else if (foundApproach)
            {
                ++nonApproachWindows;
                if (nonApproachWindows >= toleratedNonApproachWindows)
                {
                    break;
                }
            }
        }

        if (!foundApproach || earliestApproachFrame == INDEX_NONE)
        {
            errorMessage = TEXT(
                "No sustained movement toward the representative-frame location was found.");
            return false;
        }

        zeroFrame = earliestApproachFrame;
        errorMessage.Reset();
        return true;
    }

    TArray<double> motionPerFrame;
    motionPerFrame.SetNumZeroed(positions.Num());
    for (int32 frameIndex = 1; frameIndex < positions.Num(); ++frameIndex)
    {
        motionPerFrame[frameIndex] = FVector::Distance(
            positions[frameIndex],
            positions[frameIndex - 1]);
    }

    TArray<double> smoothedMotion;
    smoothedMotion.SetNumUninitialized(motionPerFrame.Num());
    for (int32 frameIndex = 0; frameIndex < motionPerFrame.Num(); ++frameIndex)
    {
        double sum = 0.0;
        int32 sampleCount = 0;
        for (int32 offset = -smoothingRadius;
             offset <= smoothingRadius;
             ++offset)
        {
            const int32 sampleIndex = frameIndex + offset;
            if (motionPerFrame.IsValidIndex(sampleIndex))
            {
                sum += motionPerFrame[sampleIndex];
                ++sampleCount;
            }
        }
        smoothedMotion[frameIndex] = sum / sampleCount;
    }

    bool foundDeparture = false;
    int32 departureSearchFrame = INDEX_NONE;
    for (int32 frameIndex = eventBoundaryFrame;
         frameIndex + progressWindowFrames < positions.Num();
         ++frameIndex)
    {
        const int32 laterFrame = frameIndex + progressWindowFrames;
        const double progressAwayFromGoal =
            smoothedDistances[laterFrame] -
            smoothedDistances[frameIndex];
        if (progressAwayFromGoal > minimumProgressCm)
        {
            foundDeparture = true;
            departureSearchFrame = laterFrame;
            break;
        }
    }

    if (!foundDeparture)
    {
        errorMessage = TEXT(
            "No sustained movement away from the representative-frame location was found.");
        return false;
    }

    constexpr int32 requiredLowMotionFrames = 3;
    constexpr double minimumLowMotionThresholdCmPerFrame = 0.02;
    constexpr double peakMotionFraction = 0.15;
    double peakMotion = 0.0;
    int32 lowMotionFrames = 0;
    for (int32 frameIndex = departureSearchFrame;
         frameIndex < smoothedMotion.Num();
         ++frameIndex)
    {
        peakMotion = FMath::Max(peakMotion, smoothedMotion[frameIndex]);
        const double lowMotionThreshold = FMath::Max(
            minimumLowMotionThresholdCmPerFrame,
            peakMotion * peakMotionFraction);
        if (smoothedMotion[frameIndex] <= lowMotionThreshold)
        {
            ++lowMotionFrames;
            if (lowMotionFrames >= requiredLowMotionFrames)
            {
                const int32 firstLowMotionFrame =
                    frameIndex - requiredLowMotionFrames + 1;
                zeroFrame = FMath::Max(
                    eventBoundaryFrame,
                    firstLowMotionFrame - 1);
                errorMessage.Reset();
                return true;
            }
        }
        else
        {
            lowMotionFrames = 0;
        }
    }

    errorMessage = TEXT(
        "Movement away was found, but no sustained low-motion interval followed it.");
    return false;
}
