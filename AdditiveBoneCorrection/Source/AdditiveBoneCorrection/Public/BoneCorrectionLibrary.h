#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BoneCorrectionTypes.h"
#include "BoneCorrectionLibrary.generated.h"

UCLASS()
class ADDITIVEBONECORRECTION_API UBoneCorrectionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Bone Correction|IK")
    static FBoneCorrectionIkResult solvePositionIk(
        const TArray<FBoneCorrectionBoneTransform>& componentSpaceBoneTransforms,
        FVector componentSpaceEffectorPoint,
        FVector componentSpaceGoal,
        const FBoneCorrectionIkSettings& settings);
};
