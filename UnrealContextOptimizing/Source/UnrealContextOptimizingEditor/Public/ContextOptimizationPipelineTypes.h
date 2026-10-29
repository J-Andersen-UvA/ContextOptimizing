#pragma once

#include "BoneCorrectionTypes.h"
#include "ContactEventTypes.h"
#include "CoreMinimal.h"
#include "SemanticPatchProximityTypes.h"
#include "ContextOptimizationPipelineTypes.generated.h"

class UAdditiveBoneCorrectionLayer;
class UAnimSequence;

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZINGEDITOR_API FContextOptimizationPipelineSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FName sourceMovementBone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FPatchProximitySettings proximitySettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FTargetSemanticEventEvaluationSettings targetEvaluationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FBoneCorrectionChain targetIkChain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FBoneCorrectionIkSettings ikSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization", meta = (ClampMin = "0"))
    int32 maximumFrameDistance = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization", meta = (Units = "cm"))
    double targetNormalOffset = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    bool onlyCorrectWhenNeeded = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FString outputPackagePath = TEXT("/Game/ContextOptimization/Output");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Optimization")
    FString outputAssetName;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZINGEDITOR_API FContextOptimizationEventCorrection
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    FMappedSemanticPatchEvent mappedEvent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    bool applied = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 sourceApproachStartFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 sourceRecoveryEndFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 targetApproachStartFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 targetEventStartFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 targetRepresentativeFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 targetEventEndFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    int32 targetRecoveryEndFrame = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    FBoneCorrectionIkResult solveResult;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    FString errorMessage;
};

USTRUCT(BlueprintType)
struct UNREALCONTEXTOPTIMIZINGEDITOR_API FContextOptimizationPipelineResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    bool success = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    TObjectPtr<UAdditiveBoneCorrectionLayer> correctionLayer = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    TObjectPtr<UAnimSequence> animation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    TArray<FContextOptimizationEventCorrection> eventCorrections;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Context Optimization")
    FString errorMessage;
};
