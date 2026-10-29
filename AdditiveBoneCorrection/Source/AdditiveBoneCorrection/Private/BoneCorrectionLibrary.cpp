#include "BoneCorrectionLibrary.h"

namespace
{
FQuat clampRotation(const FQuat& rotation, const double maximumRadians)
{
    FVector axis;
    double angle = 0.0;
    rotation.ToAxisAndAngle(axis, angle);
    if (angle > PI)
    {
        angle -= 2.0 * PI;
    }
    if (FMath::Abs(angle) <= maximumRadians || axis.IsNearlyZero())
    {
        return rotation.GetNormalized();
    }
    return FQuat(axis.GetSafeNormal(), FMath::Clamp(angle, -maximumRadians, maximumRadians));
}
}

FBoneCorrectionIkResult UBoneCorrectionLibrary::solvePositionIk(
    const TArray<FBoneCorrectionBoneTransform>& componentSpaceBoneTransforms,
    const FVector componentSpaceEffectorPoint,
    const FVector componentSpaceGoal,
    const FBoneCorrectionIkSettings& settings)
{
    FBoneCorrectionIkResult result;
    if (componentSpaceBoneTransforms.IsEmpty())
    {
        result.errorMessage = TEXT("The IK chain is empty.");
        return result;
    }

    for (const FBoneCorrectionBoneTransform& bone : componentSpaceBoneTransforms)
    {
        if (bone.boneName.IsNone() || !bone.componentSpaceTransform.IsValid())
        {
            result.errorMessage = TEXT("The IK chain contains an invalid bone transform.");
            return result;
        }
    }

    result.solvedBoneTransforms = componentSpaceBoneTransforms;
    result.componentSpaceSolvedPoint = componentSpaceEffectorPoint;
    result.initialError = FVector::Distance(componentSpaceEffectorPoint, componentSpaceGoal);
    result.finalError = result.initialError;
    result.isValid = true;

    const double tolerance = FMath::Max(0.0, settings.positionTolerance);
    if (result.finalError <= tolerance)
    {
        result.reachedGoal = true;
        return result;
    }

    const int32 maximumIterations = FMath::Max(1, settings.maximumIterations);
    const double maximumRadians = FMath::DegreesToRadians(
        FMath::Clamp(settings.maximumRotationDegreesPerStep, 0.01, 180.0));

    for (int32 iteration = 0; iteration < maximumIterations; ++iteration)
    {
        bool appliedRotation = false;
        for (int32 jointIndex = result.solvedBoneTransforms.Num() - 1;
             jointIndex >= 0;
             --jointIndex)
        {
            FTransform& jointTransform =
                result.solvedBoneTransforms[jointIndex].componentSpaceTransform;
            const FVector jointPosition = jointTransform.GetTranslation();
            const FVector toEffector =
                result.componentSpaceSolvedPoint - jointPosition;
            const FVector toGoal = componentSpaceGoal - jointPosition;
            if (toEffector.IsNearlyZero() || toGoal.IsNearlyZero())
            {
                continue;
            }

            const FQuat unclampedRotation = FQuat::FindBetweenNormals(
                toEffector.GetSafeNormal(),
                toGoal.GetSafeNormal());
            const FQuat rotation = clampRotation(
                unclampedRotation,
                maximumRadians);
            if (rotation.IsIdentity(UE_KINDA_SMALL_NUMBER))
            {
                continue;
            }

            appliedRotation = true;
            jointTransform.SetRotation(
                (rotation * jointTransform.GetRotation()).GetNormalized());
            for (int32 childIndex = jointIndex + 1;
                 childIndex < result.solvedBoneTransforms.Num();
                 ++childIndex)
            {
                FTransform& childTransform =
                    result.solvedBoneTransforms[childIndex].componentSpaceTransform;
                childTransform.SetTranslation(
                    jointPosition + rotation.RotateVector(
                        childTransform.GetTranslation() - jointPosition));
                childTransform.SetRotation(
                    (rotation * childTransform.GetRotation()).GetNormalized());
            }
            result.componentSpaceSolvedPoint =
                jointPosition + rotation.RotateVector(
                    result.componentSpaceSolvedPoint - jointPosition);

            result.finalError = FVector::Distance(
                result.componentSpaceSolvedPoint,
                componentSpaceGoal);
            if (result.finalError <= tolerance)
            {
                result.iterationCount = iteration + 1;
                result.reachedGoal = true;
                return result;
            }
        }

        result.iterationCount = iteration + 1;
        if (!appliedRotation)
        {
            break;
        }
    }

    result.finalError = FVector::Distance(
        result.componentSpaceSolvedPoint,
        componentSpaceGoal);
    result.reachedGoal = result.finalError <= tolerance;
    return result;
}
