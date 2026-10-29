#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BoneCorrectionTypes.h"
#include "BoneCorrectionEditorLibrary.generated.h"

class UAnimSequence;
class UAnimSequenceBase;
class USkeletalMesh;

USTRUCT(BlueprintType)
struct ADDITIVEBONECORRECTIONEDITOR_API FBoneCorrectionBakeResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    bool success = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    TObjectPtr<UAnimSequence> animation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bone Correction")
    FString errorMessage;
};

UCLASS()
class ADDITIVEBONECORRECTIONEDITOR_API UBoneCorrectionEditorLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Bone Correction|IK")
    static FBoneCorrectionIkResult solvePositionIkAtFrame(
        const UAnimSequenceBase* animation,
        const USkeletalMesh* skeletalMesh,
        int32 frame,
        FName interactionBone,
        FVector interactionBoneLocalPosition,
        FVector componentSpaceGoal,
        const FBoneCorrectionChain& chain,
        const FBoneCorrectionIkSettings& settings);

    UFUNCTION(BlueprintCallable, Category = "Bone Correction|Layer")
    static UAdditiveBoneCorrectionLayer* createCorrectionLayerAsset(
        UAnimSequence* baseAnimation,
        const FString& outputPackagePath,
        const FString& outputAssetName,
        FString& errorMessage);

    UFUNCTION(BlueprintCallable, Category = "Bone Correction|Layer")
    static bool addCorrectionKey(
        UAdditiveBoneCorrectionLayer* layer,
        const FBoneCorrectionIkResult& solveResult,
        FString& errorMessage);

    UFUNCTION(BlueprintCallable, Category = "Bone Correction|Bake")
    static FBoneCorrectionBakeResult bakeCorrectionLayer(
        UAdditiveBoneCorrectionLayer* layer,
        const USkeletalMesh* skeletalMesh,
        const FString& outputPackagePath,
        const FString& outputAssetName);
};
