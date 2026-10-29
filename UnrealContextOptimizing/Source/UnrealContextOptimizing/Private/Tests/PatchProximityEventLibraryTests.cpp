#include "PatchProximityEventLibrary.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FPatchProximityResult makeResult(
    const FName pointId,
    const FName patchId,
    const double distance = 0.0,
    const FVector2D normalizedPatchLocation = FVector2D::ZeroVector)
{
    FPatchProximityResult result;
    result.pointId = pointId;
    result.interactionGroupId = pointId;
    result.patchId = patchId;
    result.distance = distance;
    result.signedDistance = distance;
    result.normalizedPatchLocation = normalizedPatchLocation;
    return result;
}

FPatchProximityFrame makeFrame(
    const int32 frameIndex,
    std::initializer_list<FPatchProximityResult> results)
{
    FPatchProximityFrame frame;
    frame.frameIndex = frameIndex;
    frame.timeSeconds = static_cast<double>(frameIndex) / 30.0;
    for (const FPatchProximityResult& result : results)
    {
        frame.results.Add(result);
    }
    return frame;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPatchProximityFrameNeighborsTest,
    "ContextOptimizing.SourceEvents.FrameNeighbors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatchProximityFrameNeighborsTest::RunTest(const FString& parameters)
{
    FPatchProximityAnimationData animationData;
    animationData.isValid = true;
    animationData.frames = {
        makeFrame(10, {makeResult("rightThumbTip", "rightCheek")}),
        makeFrame(11, {
            makeResult("rightThumbTip", "rightCheek"),
            makeResult("rightThumbTip", "rightZygoma")}),
        makeFrame(12, {}),
        makeFrame(13, {makeResult("rightThumbTip", "rightCheek")})};

    const TArray<FPatchProximityFrameNeighborGroup> strictGroups =
        UPatchProximityEventLibrary::getPatchProximityFrameNeighbors(
            animationData,
            1);
    TestEqual(TEXT("One-frame distance creates two groups"), strictGroups.Num(), 2);
    TestEqual(TEXT("First group starts at frame 10"), strictGroups[0].startFrame, 10);
    TestEqual(TEXT("First group ends at frame 11"), strictGroups[0].endFrame, 11);
    TestEqual(TEXT("Second group starts at frame 13"), strictGroups[1].startFrame, 13);

    const TArray<FPatchProximityFrameNeighborGroup> tolerantGroups =
        UPatchProximityEventLibrary::getPatchProximityFrameNeighbors(
            animationData,
            2);
    TestEqual(TEXT("Two-frame distance bridges the empty frame"), tolerantGroups.Num(), 1);
    TestEqual(TEXT("Tolerant group retains populated frames only"), tolerantGroups[0].frames.Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPatchProximityPointNeighborsTest,
    "ContextOptimizing.SourceEvents.PointNeighbors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatchProximityPointNeighborsTest::RunTest(const FString& parameters)
{
    FPatchProximityFrameNeighborGroup frameGroup;
    frameGroup.startFrame = 20;
    frameGroup.endFrame = 21;
    frameGroup.frames = {
        makeFrame(20, {
            makeResult("rightThumbTip", "rightCheek"),
            makeResult("rightThumbTip", "rightZygoma")}),
        makeFrame(21, {
            makeResult("rightThumbTip", "rightCheek"),
            makeResult("rightIndexTip", "rightZygoma")})};

    const TArray<FPatchProximityPointNeighborGroup> pointGroups =
        UPatchProximityEventLibrary::getPatchProximityPointNeighbors(frameGroup);

    TestEqual(TEXT("Results are split into two point groups"), pointGroups.Num(), 2);
    TestEqual(TEXT("First point group is the first observed point"), pointGroups[0].pointId, FName("rightThumbTip"));
    TestEqual(TEXT("Thumb group spans both frames"), pointGroups[0].frames.Num(), 2);
    TestEqual(TEXT("Overlapping patch matches are retained"), pointGroups[0].frames[0].results.Num(), 2);
    TestEqual(TEXT("Index group contains its one frame"), pointGroups[1].frames.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBuildSourceSemanticPatchEventTest,
    "ContextOptimizing.SourceEvents.BuildSemanticPatchEvent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuildSourceSemanticPatchEventTest::RunTest(const FString& parameters)
{
    FPatchProximityPointNeighborGroup pointGroup;
    pointGroup.pointId = TEXT("rightIndexTip");
    pointGroup.interactionGroupId = TEXT("rightIndex");
    pointGroup.startFrame = 30;
    pointGroup.endFrame = 31;
    pointGroup.frames = {
        makeFrame(30, {
            makeResult("rightIndexTip", "rightCheek", 0.3, FVector2D(0.8, 0.7)),
            makeResult("rightIndexTip", "rightZygoma", 0.3, FVector2D(0.1, 0.2))}),
        makeFrame(31, {
            makeResult("rightIndexTip", "rightCheek", 0.2, FVector2D(0.7, 0.6)),
            makeResult("rightIndexTip", "rightZygoma", 0.1, FVector2D(0.2, 0.1))})};

    const FSourceSemanticPatchEvent semanticEvent =
        UPatchProximityEventLibrary::buildSourceSemanticPatchEvent(pointGroup);

    TestTrue(TEXT("Event is valid"), semanticEvent.isValid);
    TestEqual(TEXT("Patch nearest its semantic centre wins"), semanticEvent.patchId, FName("rightZygoma"));
    TestEqual(TEXT("Closest approach is representative"), semanticEvent.representativeFrame, 31);
    TestEqual(TEXT("Representative time is retained"), semanticEvent.representativeTimeSeconds, 31.0 / 30.0);
    TestEqual(TEXT("Selected-patch frames are retained"), semanticEvent.frames.Num(), 2);
    TestEqual(TEXT("Other overlapping patches are removed"), semanticEvent.frames[0].results.Num(), 1);
    TestEqual(TEXT("Representative location is retained"), semanticEvent.normalizedPatchLocation, FVector2D(0.2, 0.1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEvaluateTargetSemanticPatchEventTest,
    "ContextOptimizing.SourceEvents.EvaluateTargetSemanticPatchEvent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaluateTargetSemanticPatchEventTest::RunTest(const FString& parameters)
{
    FSourceSemanticPatchEvent sourceEvent;
    sourceEvent.isValid = true;
    sourceEvent.pointId = TEXT("rightIndexTip");

    FPatchProximityTarget target;
    target.isValid = true;
    target.componentSpaceInteractionGoal = FVector(1.0, 2.0, 0.0);
    target.componentSpaceSurfaceNormal = FVector::ForwardVector;

    FSemanticPose targetPose;
    targetPose.componentSpaceBoneTransforms.Add(
        TEXT("indexTip"),
        FTransform::Identity);

    FSemanticInteractionPoint point;
    point.enabled = true;
    point.pointId = TEXT("rightIndexTip");
    point.parentBone = TEXT("indexTip");
    point.boneLocalPosition = FVector(0.5, 0.5, 0.0);

    FTargetSemanticEventEvaluationSettings settings;
    settings.normalTolerance = 0.25;
    settings.tangentialTolerance = 1.0;

    const FTargetSemanticEventEvaluation evaluation =
        UPatchProximityEventLibrary::evaluateTargetSemanticPatchEvent(
            sourceEvent,
            target,
            targetPose,
            {point},
            settings);

    TestTrue(TEXT("Evaluation is valid"), evaluation.isValid);
    TestEqual(
        TEXT("Current point comes from the matching target point"),
        evaluation.componentSpaceCurrentPoint,
        FVector(0.5, 0.5, 0.0));
    TestEqual(
        TEXT("Correction vector points from current point to goal"),
        evaluation.componentSpaceError,
        FVector(0.5, 1.5, 0.0));
    TestTrue(
        TEXT("Normal error is signed"),
        FMath::IsNearlyEqual(evaluation.normalError, 0.5));
    TestTrue(
        TEXT("Tangential magnitude is measured"),
        FMath::IsNearlyEqual(evaluation.tangentialError, 1.5));
    TestTrue(
        TEXT("Tangential tolerance requests correction"),
        evaluation.needsCorrection);
    return true;
}

#endif
