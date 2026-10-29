#include "BoneCorrectionEditorLibrary.h"

#include "AnimPose.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BoneCorrectionLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "BoneCorrectionEditorLibrary"

namespace
{
bool setError(FString& errorMessage, const FString& value)
{
    errorMessage = value;
    return false;
}

FString makePackageName(const FString& packagePath, const FString& assetName)
{
    FString normalizedPath = packagePath;
    normalizedPath.RemoveFromEnd(TEXT("/"));
    return normalizedPath / assetName;
}

FAnimPoseEvaluationOptions makeEvaluationOptions(const USkeletalMesh* skeletalMesh)
{
    FAnimPoseEvaluationOptions options;
    options.EvaluationType = EAnimDataEvalType::Raw;
    options.bShouldRetarget = true;
    options.bEvaluateCurves = false;
    options.OptionalSkeletalMesh = const_cast<USkeletalMesh*>(skeletalMesh);
    return options;
}

bool evaluatePoseAtFrame(
    const UAnimSequenceBase& animation,
    const USkeletalMesh& skeletalMesh,
    const int32 frame,
    FAnimPose& outPose,
    FString& errorMessage)
{
    const int32 frameCount = animation.GetNumberOfSampledKeys();
    const FFrameRate frameRate = animation.GetSamplingFrameRate();
    if (frame < 0 || frame >= frameCount || !frameRate.IsValid())
    {
        return setError(errorMessage, TEXT("The requested animation frame is invalid."));
    }

    UAnimPoseExtensions::GetAnimPoseAtTime(
        &animation,
        frameRate.AsSeconds(FFrameTime(frame)),
        makeEvaluationOptions(&skeletalMesh),
        outPose);
    if (!UAnimPoseExtensions::IsValid(outPose))
    {
        return setError(errorMessage, TEXT("Unreal could not evaluate the animation pose."));
    }
    return true;
}

bool validateChain(
    const USkeletalMesh& skeletalMesh,
    const FBoneCorrectionChain& chain,
    FString& errorMessage)
{
    if (chain.bones.IsEmpty())
    {
        return setError(errorMessage, TEXT("The IK chain is empty."));
    }

    const FReferenceSkeleton& referenceSkeleton = skeletalMesh.GetRefSkeleton();
    for (int32 index = 0; index < chain.bones.Num(); ++index)
    {
        const int32 boneIndex = referenceSkeleton.FindBoneIndex(chain.bones[index]);
        if (boneIndex == INDEX_NONE)
        {
            return setError(
                errorMessage,
                FString::Printf(
                    TEXT("The IK bone '%s' does not exist on the skeletal mesh."),
                    *chain.bones[index].ToString()));
        }
        if (index > 0)
        {
            const int32 expectedParentIndex =
                referenceSkeleton.FindBoneIndex(chain.bones[index - 1]);
            if (referenceSkeleton.GetParentIndex(boneIndex) != expectedParentIndex)
            {
                return setError(
                    errorMessage,
                    FString::Printf(
                        TEXT("The IK chain must be ordered and contiguous. '%s' is not the direct parent of '%s'."),
                        *chain.bones[index - 1].ToString(),
                        *chain.bones[index].ToString()));
            }
        }
    }
    return true;
}

const FBoneCorrectionBoneTransform* findSolvedBone(
    const TArray<FBoneCorrectionBoneTransform>& transforms,
    const FName boneName)
{
    return transforms.FindByPredicate(
        [boneName](const FBoneCorrectionBoneTransform& transform)
        {
            return transform.boneName == boneName;
        });
}

const FBoneCorrectionBoneDelta* findDelta(
    const TArray<FBoneCorrectionBoneDelta>& deltas,
    const FName boneName)
{
    return deltas.FindByPredicate(
        [boneName](const FBoneCorrectionBoneDelta& delta)
        {
            return delta.boneName == boneName;
        });
}

UAnimSequence* duplicateAnimation(
    UAnimSequence& source,
    const FString& packageName,
    const FString& assetName,
    FString& errorMessage)
{
    FText reason;
    if (!FPackageName::IsValidLongPackageName(packageName, true, &reason))
    {
        errorMessage = FString::Printf(
            TEXT("Invalid output package '%s': %s"),
            *packageName,
            *reason.ToString());
        return nullptr;
    }
    if (FPackageName::DoesPackageExist(packageName) ||
        FindPackage(nullptr, *packageName) != nullptr)
    {
        errorMessage = FString::Printf(
            TEXT("An asset package already exists at '%s'."),
            *packageName);
        return nullptr;
    }

    UPackage* package = CreatePackage(*packageName);
    UAnimSequence* output = DuplicateObject<UAnimSequence>(
        &source,
        package,
        FName(*assetName));
    if (output == nullptr)
    {
        errorMessage = TEXT("Unreal could not duplicate the base animation.");
        return nullptr;
    }
    output->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
    return output;
}
}

FBoneCorrectionIkResult UBoneCorrectionEditorLibrary::solvePositionIkAtFrame(
    const UAnimSequenceBase* animation,
    const USkeletalMesh* skeletalMesh,
    const int32 frame,
    const FName interactionBone,
    const FVector interactionBoneLocalPosition,
    const FVector componentSpaceGoal,
    const FBoneCorrectionChain& chain,
    const FBoneCorrectionIkSettings& settings)
{
    FBoneCorrectionIkResult result;
    result.correctionKey.frame = frame;
    if (animation == nullptr || skeletalMesh == nullptr)
    {
        result.errorMessage = TEXT("Animation and skeletal mesh are required.");
        return result;
    }
    if (!validateChain(*skeletalMesh, chain, result.errorMessage))
    {
        return result;
    }
    if (skeletalMesh->GetRefSkeleton().FindBoneIndex(interactionBone) == INDEX_NONE)
    {
        result.errorMessage = TEXT("The interaction bone does not exist on the skeletal mesh.");
        return result;
    }

    FAnimPose pose;
    if (!evaluatePoseAtFrame(
            *animation,
            *skeletalMesh,
            frame,
            pose,
            result.errorMessage))
    {
        return result;
    }

    const FTransform interactionBoneTransform = UAnimPoseExtensions::GetBonePose(
        pose,
        interactionBone,
        EAnimPoseSpaces::World);
    const FVector componentSpaceInteractionPoint =
        interactionBoneTransform.TransformPosition(interactionBoneLocalPosition);

    TArray<FBoneCorrectionBoneTransform> chainTransforms;
    chainTransforms.Reserve(chain.bones.Num());
    for (const FName boneName : chain.bones)
    {
        FBoneCorrectionBoneTransform& transform = chainTransforms.AddDefaulted_GetRef();
        transform.boneName = boneName;
        transform.componentSpaceTransform = UAnimPoseExtensions::GetBonePose(
            pose,
            boneName,
            EAnimPoseSpaces::World);
    }

    result = UBoneCorrectionLibrary::solvePositionIk(
        chainTransforms,
        componentSpaceInteractionPoint,
        componentSpaceGoal,
        settings);
    result.correctionKey.frame = frame;
    if (!result.isValid)
    {
        return result;
    }

    const FReferenceSkeleton& referenceSkeleton = skeletalMesh->GetRefSkeleton();
    result.correctionKey.boneDeltas.Reserve(chain.bones.Num());
    for (const FName boneName : chain.bones)
    {
        const FBoneCorrectionBoneTransform* solvedBone = findSolvedBone(
            result.solvedBoneTransforms,
            boneName);
        if (solvedBone == nullptr)
        {
            result.isValid = false;
            result.errorMessage = TEXT("The IK solver did not return every chain bone.");
            result.correctionKey.boneDeltas.Reset();
            return result;
        }

        const int32 boneIndex = referenceSkeleton.FindBoneIndex(boneName);
        const int32 parentIndex = referenceSkeleton.GetParentIndex(boneIndex);
        FTransform parentComponentTransform = FTransform::Identity;
        if (parentIndex != INDEX_NONE)
        {
            const FName parentName = referenceSkeleton.GetBoneName(parentIndex);
            if (const FBoneCorrectionBoneTransform* solvedParent = findSolvedBone(
                    result.solvedBoneTransforms,
                    parentName))
            {
                parentComponentTransform = solvedParent->componentSpaceTransform;
            }
            else
            {
                parentComponentTransform = UAnimPoseExtensions::GetBonePose(
                    pose,
                    parentName,
                    EAnimPoseSpaces::World);
            }
        }

        const FTransform solvedLocal = solvedBone->componentSpaceTransform
            .GetRelativeTransform(parentComponentTransform);
        const FTransform baseLocal = UAnimPoseExtensions::GetBonePose(
            pose,
            boneName,
            EAnimPoseSpaces::Local);

        FBoneCorrectionBoneDelta& delta =
            result.correctionKey.boneDeltas.AddDefaulted_GetRef();
        delta.boneName = boneName;
        delta.translationDelta =
            solvedLocal.GetTranslation() - baseLocal.GetTranslation();
        delta.rotationDelta =
            (solvedLocal.GetRotation() * baseLocal.GetRotation().Inverse())
              .GetNormalized();
        const FVector baseScale = baseLocal.GetScale3D();
        const FVector solvedScale = solvedLocal.GetScale3D();
        delta.scaleMultiplier = FVector(
            FMath::IsNearlyZero(baseScale.X) ? 1.0 : solvedScale.X / baseScale.X,
            FMath::IsNearlyZero(baseScale.Y) ? 1.0 : solvedScale.Y / baseScale.Y,
            FMath::IsNearlyZero(baseScale.Z) ? 1.0 : solvedScale.Z / baseScale.Z);
    }
    return result;
}

UAdditiveBoneCorrectionLayer*
UBoneCorrectionEditorLibrary::createCorrectionLayerAsset(
    UAnimSequence* baseAnimation,
    const FString& outputPackagePath,
    const FString& outputAssetName,
    FString& errorMessage)
{
    errorMessage.Reset();
    if (baseAnimation == nullptr || outputAssetName.IsEmpty())
    {
        setError(errorMessage, TEXT("Base animation and output asset name are required."));
        return nullptr;
    }

    const FString packageName = makePackageName(
        outputPackagePath,
        outputAssetName);
    FText reason;
    if (!FPackageName::IsValidLongPackageName(packageName, true, &reason))
    {
        setError(
            errorMessage,
            FString::Printf(TEXT("Invalid output package: %s"), *reason.ToString()));
        return nullptr;
    }
    if (FPackageName::DoesPackageExist(packageName) ||
        FindPackage(nullptr, *packageName) != nullptr)
    {
        setError(errorMessage, TEXT("An asset already exists at the output package."));
        return nullptr;
    }

    UPackage* package = CreatePackage(*packageName);
    UAdditiveBoneCorrectionLayer* layer =
        NewObject<UAdditiveBoneCorrectionLayer>(
            package,
            FName(*outputAssetName),
            RF_Public | RF_Standalone | RF_Transactional);
    if (layer == nullptr)
    {
        setError(errorMessage, TEXT("Unreal could not create the correction layer."));
        return nullptr;
    }
    layer->baseAnimation = baseAnimation;
    layer->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(layer);
    return layer;
}

bool UBoneCorrectionEditorLibrary::addCorrectionKey(
    UAdditiveBoneCorrectionLayer* layer,
    const FBoneCorrectionIkResult& solveResult,
    FString& errorMessage)
{
    errorMessage.Reset();
    if (layer == nullptr)
    {
        return setError(errorMessage, TEXT("A correction layer is required."));
    }
    if (!solveResult.isValid || solveResult.correctionKey.boneDeltas.IsEmpty())
    {
        return setError(errorMessage, TEXT("The IK result does not contain a valid correction key."));
    }
    layer->addOrUpdateKey(solveResult.correctionKey);
    return true;
}

FBoneCorrectionBakeResult UBoneCorrectionEditorLibrary::bakeCorrectionLayer(
    UAdditiveBoneCorrectionLayer* layer,
    const USkeletalMesh* skeletalMesh,
    const FString& outputPackagePath,
    const FString& outputAssetName)
{
    FBoneCorrectionBakeResult result;
    if (layer == nullptr || skeletalMesh == nullptr)
    {
        result.errorMessage = TEXT("Correction layer and skeletal mesh are required.");
        return result;
    }
    UAnimSequence* baseAnimation = layer->baseAnimation.LoadSynchronous();
    if (baseAnimation == nullptr)
    {
        result.errorMessage = TEXT("The correction layer has no valid base animation.");
        return result;
    }
    if (layer->keys.IsEmpty())
    {
        result.errorMessage = TEXT("The correction layer has no keys.");
        return result;
    }

    TSet<FName> correctedBoneNames;
    for (const FAdditiveBoneCorrectionKey& key : layer->keys)
    {
        for (const FBoneCorrectionBoneDelta& delta : key.boneDeltas)
        {
            correctedBoneNames.Add(delta.boneName);
        }
    }
    if (correctedBoneNames.IsEmpty())
    {
        result.errorMessage = TEXT("The correction layer has no bone deltas.");
        return result;
    }

    const int32 frameCount = baseAnimation->GetNumberOfSampledKeys();
    TMap<FName, TArray<FVector>> translationsByBone;
    TMap<FName, TArray<FQuat>> rotationsByBone;
    TMap<FName, TArray<FVector>> scalesByBone;
    for (const FName boneName : correctedBoneNames)
    {
        translationsByBone.Add(boneName).Reserve(frameCount);
        rotationsByBone.Add(boneName).Reserve(frameCount);
        scalesByBone.Add(boneName).Reserve(frameCount);
    }

    for (int32 frame = 0; frame < frameCount; ++frame)
    {
        FAnimPose pose;
        if (!evaluatePoseAtFrame(
                *baseAnimation,
                *skeletalMesh,
                frame,
                pose,
                result.errorMessage))
        {
            return result;
        }

        TArray<FBoneCorrectionBoneDelta> frameDeltas;
        layer->evaluateBoneDeltas(frame, frameDeltas);
        for (const FName boneName : correctedBoneNames)
        {
            FTransform localTransform = UAnimPoseExtensions::GetBonePose(
                pose,
                boneName,
                EAnimPoseSpaces::Local);
            if (const FBoneCorrectionBoneDelta* delta = findDelta(
                    frameDeltas,
                    boneName))
            {
                localTransform.AddToTranslation(delta->translationDelta);
                localTransform.SetRotation(
                    (delta->rotationDelta * localTransform.GetRotation())
                      .GetNormalized());
                localTransform.SetScale3D(
                    localTransform.GetScale3D() * delta->scaleMultiplier);
            }
            translationsByBone.FindChecked(boneName).Add(
                localTransform.GetTranslation());
            rotationsByBone.FindChecked(boneName).Add(
                localTransform.GetRotation());
            scalesByBone.FindChecked(boneName).Add(
                localTransform.GetScale3D());
        }
    }

    const FString packageName = makePackageName(
        outputPackagePath,
        outputAssetName);
    UAnimSequence* output = duplicateAnimation(
        *baseAnimation,
        packageName,
        outputAssetName,
        result.errorMessage);
    if (output == nullptr)
    {
        return result;
    }

    IAnimationDataModel* dataModel = output->GetDataModel();
    IAnimationDataController& controller = output->GetController();
    IAnimationDataController::FScopedBracket bracket(
        controller,
        LOCTEXT("BakeBoneCorrections", "Bake additive bone corrections"),
        true);
    for (const FName boneName : correctedBoneNames)
    {
        if (!dataModel->IsValidBoneTrackName(boneName) &&
            !controller.AddBoneCurve(boneName, true))
        {
            result.errorMessage = FString::Printf(
                TEXT("Could not add the animation track for '%s'."),
                *boneName.ToString());
            output->ClearFlags(RF_Public | RF_Standalone);
            return result;
        }
        if (!controller.SetBoneTrackKeys(
                boneName,
                translationsByBone.FindChecked(boneName),
                rotationsByBone.FindChecked(boneName),
                scalesByBone.FindChecked(boneName),
                true))
        {
            result.errorMessage = FString::Printf(
                TEXT("Could not write the animation track for '%s'."),
                *boneName.ToString());
            output->ClearFlags(RF_Public | RF_Standalone);
            return result;
        }
    }

    output->MarkPackageDirty();
    output->PostEditChange();
    FAssetRegistryModule::AssetCreated(output);
    result.success = true;
    result.animation = output;
    return result;
}

#undef LOCTEXT_NAMESPACE
