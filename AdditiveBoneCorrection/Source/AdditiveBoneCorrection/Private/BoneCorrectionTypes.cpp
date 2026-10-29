#include "BoneCorrectionTypes.h"

namespace
{
FBoneCorrectionBoneDelta identityDelta(const FName boneName)
{
    FBoneCorrectionBoneDelta delta;
    delta.boneName = boneName;
    return delta;
}

const FBoneCorrectionBoneDelta* findDelta(
    const FAdditiveBoneCorrectionKey& key,
    const FName boneName)
{
    return key.boneDeltas.FindByPredicate(
        [boneName](const FBoneCorrectionBoneDelta& delta)
        {
            return delta.boneName == boneName;
        });
}
}

void UAdditiveBoneCorrectionLayer::addOrUpdateKey(
    const FAdditiveBoneCorrectionKey& key)
{
    Modify();
    if (FAdditiveBoneCorrectionKey* existing = keys.FindByPredicate(
            [&key](const FAdditiveBoneCorrectionKey& candidate)
            {
                return candidate.frame == key.frame;
            }))
    {
        *existing = key;
    }
    else
    {
        keys.Add(key);
    }
    keys.Sort(
        [](const FAdditiveBoneCorrectionKey& left,
           const FAdditiveBoneCorrectionKey& right)
        {
            return left.frame < right.frame;
        });
    MarkPackageDirty();
}

bool UAdditiveBoneCorrectionLayer::removeKey(const int32 frame)
{
    const int32 index = keys.IndexOfByPredicate(
        [frame](const FAdditiveBoneCorrectionKey& key)
        {
            return key.frame == frame;
        });
    if (index == INDEX_NONE)
    {
        return false;
    }
    Modify();
    keys.RemoveAt(index);
    MarkPackageDirty();
    return true;
}

void UAdditiveBoneCorrectionLayer::evaluateBoneDeltas(
    const int32 frame,
    TArray<FBoneCorrectionBoneDelta>& outDeltas) const
{
    outDeltas.Reset();
    if (keys.IsEmpty() || frame < keys[0].frame || frame > keys.Last().frame)
    {
        return;
    }

    const FAdditiveBoneCorrectionKey* previous = &keys[0];
    const FAdditiveBoneCorrectionKey* next = &keys.Last();
    for (const FAdditiveBoneCorrectionKey& key : keys)
    {
        if (key.frame <= frame)
        {
            previous = &key;
        }
        if (key.frame >= frame)
        {
            next = &key;
            break;
        }
    }

    TSet<FName> boneNames;
    for (const FBoneCorrectionBoneDelta& delta : previous->boneDeltas)
    {
        boneNames.Add(delta.boneName);
    }
    for (const FBoneCorrectionBoneDelta& delta : next->boneDeltas)
    {
        boneNames.Add(delta.boneName);
    }

    double alpha = 0.0;
    if (next->frame > previous->frame)
    {
        alpha = static_cast<double>(frame - previous->frame) /
            static_cast<double>(next->frame - previous->frame);
    }
    if (useCubicInterpolation)
    {
        alpha = alpha * alpha * (3.0 - 2.0 * alpha);
    }

    outDeltas.Reserve(boneNames.Num());
    for (const FName boneName : boneNames)
    {
        const FBoneCorrectionBoneDelta previousIdentity = identityDelta(boneName);
        const FBoneCorrectionBoneDelta nextIdentity = identityDelta(boneName);
        const FBoneCorrectionBoneDelta* previousDelta = findDelta(*previous, boneName);
        const FBoneCorrectionBoneDelta* nextDelta = findDelta(*next, boneName);
        if (previousDelta == nullptr)
        {
            previousDelta = &previousIdentity;
        }
        if (nextDelta == nullptr)
        {
            nextDelta = &nextIdentity;
        }

        FBoneCorrectionBoneDelta& result = outDeltas.AddDefaulted_GetRef();
        result.boneName = boneName;
        result.translationDelta = FMath::Lerp(
            previousDelta->translationDelta,
            nextDelta->translationDelta,
            alpha);
        result.rotationDelta = FQuat::Slerp(
            previousDelta->rotationDelta,
            nextDelta->rotationDelta,
            alpha).GetNormalized();
        result.scaleMultiplier = FMath::Lerp(
            previousDelta->scaleMultiplier,
            nextDelta->scaleMultiplier,
            alpha);
    }
}
