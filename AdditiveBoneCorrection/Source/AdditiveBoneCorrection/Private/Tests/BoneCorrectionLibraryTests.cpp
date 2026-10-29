#if WITH_DEV_AUTOMATION_TESTS

#include "BoneCorrectionLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBoneCorrectionPositionIkTest,
    "AdditiveBoneCorrection.IK.ReachesPositionWithSelectedChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBoneCorrectionPositionIkTest::RunTest(const FString& parameters)
{
    TArray<FBoneCorrectionBoneTransform> chain;
    chain.SetNum(3);
    chain[0].boneName = TEXT("upperArm");
    chain[0].componentSpaceTransform.SetTranslation(FVector(0.0, 0.0, 0.0));
    chain[1].boneName = TEXT("lowerArm");
    chain[1].componentSpaceTransform.SetTranslation(FVector(5.0, 0.0, 0.0));
    chain[2].boneName = TEXT("hand");
    chain[2].componentSpaceTransform.SetTranslation(FVector(10.0, 0.0, 0.0));

    FBoneCorrectionIkSettings settings;
    settings.maximumIterations = 64;
    settings.positionTolerance = 0.01;
    settings.maximumRotationDegreesPerStep = 30.0;
    const FBoneCorrectionIkResult result = UBoneCorrectionLibrary::solvePositionIk(
        chain,
        FVector(12.0, 0.0, 0.0),
        FVector(8.0, 7.0, 0.0),
        settings);

    TestTrue(TEXT("The solve is valid"), result.isValid);
    TestTrue(TEXT("The selected chain reaches the goal"), result.reachedGoal);
    TestTrue(TEXT("The residual is within tolerance"), result.finalError <= settings.positionTolerance);
    return true;
}

#endif
