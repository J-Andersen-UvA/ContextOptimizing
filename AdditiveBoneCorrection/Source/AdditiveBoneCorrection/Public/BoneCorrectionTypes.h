#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BoneCorrectionTypes.generated.h"

class UAnimSequence;

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTION_API FBoneCorrectionChain
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction")
    TArray<FName> bones;
};

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTION_API FBoneCorrectionIkSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction", meta = (ClampMin = "1"))
    int32 maximumIterations = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction", meta = (ClampMin = "0.0", Units = "cm"))
    double positionTolerance = 0.05;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction", meta = (ClampMin = "0.01", ClampMax = "180.0", Units = "deg"))
    double maximumRotationDegreesPerStep = 30.0;
};

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTION_API FBoneCorrectionBoneTransform
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    FName boneName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    FTransform componentSpaceTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTION_API FBoneCorrectionBoneDelta
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction")
    FName boneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction", meta = (Units = "cm"))
    FVector translationDelta = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction")
    FQuat rotationDelta = FQuat::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction")
    FVector scaleMultiplier = FVector::OneVector;
};

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTION_API FAdditiveBoneCorrectionKey
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction", meta = (ClampMin = "0"))
    int32 frame = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Correction")
    TArray<FBoneCorrectionBoneDelta> boneDeltas;
};

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTION_API FBoneCorrectionIkResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    bool isValid = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    bool reachedGoal = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    int32 iterationCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction", meta = (Units = "cm"))
    double initialError = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction", meta = (Units = "cm"))
    double finalError = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    FVector componentSpaceSolvedPoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    TArray<FBoneCorrectionBoneTransform> solvedBoneTransforms;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    FAdditiveBoneCorrectionKey correctionKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    FString errorMessage;
};

UCLASS(BlueprintType)
class ADDITIVEBONECORRECTION_API UAdditiveBoneCorrectionLayer : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    TSoftObjectPtr<UAnimSequence> baseAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    bool useCubicInterpolation = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    TArray<FAdditiveBoneCorrectionKey> keys;

    UFUNCTION(BlueprintCallable, Category = "Bone Correction")
    void addOrUpdateKey(const FAdditiveBoneCorrectionKey& key);

    UFUNCTION(BlueprintCallable, Category = "Bone Correction")
    bool removeKey(int32 frame);

    void evaluateBoneDeltas(int32 frame, TArray<FBoneCorrectionBoneDelta>& outDeltas) const;
};
