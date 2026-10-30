#include "SemanticInteractionPointsComponentCustomization.h"

#include "ReferenceSkeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Actor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "SemanticInteractionPointComponent.h"
#include "SemanticInteractionPointsComponent.h"
#include "SurfacePatchEditorRefresh.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SemanticInteractionPointsComponentCustomization"

namespace
{
UBlueprint* getInteractionPointsOwningBlueprint(const UObject* object)
{
    if (object == nullptr)
    {
        return nullptr;
    }

    if (const UBlueprintGeneratedClass* generatedClass =
        object->GetTypedOuter<UBlueprintGeneratedClass>())
    {
        return Cast<UBlueprint>(generatedClass->ClassGeneratedBy);
    }
    return object->GetTypedOuter<UBlueprint>();
}

TArray<FTransform> getInteractionPointReferenceBoneTransforms(
    const FReferenceSkeleton& skeleton)
{
    const TArray<FTransform>& localTransforms = skeleton.GetRefBonePose();
    TArray<FTransform> componentTransforms;
    componentTransforms.SetNum(localTransforms.Num());
    for (int32 boneIndex = 0; boneIndex < localTransforms.Num(); ++boneIndex)
    {
        componentTransforms[boneIndex] = localTransforms[boneIndex];
        const int32 parentIndex = skeleton.GetParentIndex(boneIndex);
        if (parentIndex != INDEX_NONE)
        {
            componentTransforms[boneIndex] *= componentTransforms[parentIndex];
        }
    }
    return componentTransforms;
}

bool replaceRightWithLeft(FString& value)
{
    if (value.StartsWith(TEXT("right"), ESearchCase::CaseSensitive))
    {
        value = TEXT("left") + value.RightChop(5);
        return true;
    }
    if (value.StartsWith(TEXT("Right"), ESearchCase::CaseSensitive))
    {
        value = TEXT("Left") + value.RightChop(5);
        return true;
    }
    if (value.ReplaceInline(
            TEXT("Right"),
            TEXT("Left"),
            ESearchCase::CaseSensitive) > 0)
    {
        return true;
    }
    if (value.ReplaceInline(
            TEXT("_r"),
            TEXT("_l"),
            ESearchCase::CaseSensitive) > 0)
    {
        return true;
    }
    return false;
}

FVector mirrorVector(FVector value, const EAxis::Type mirrorAxis)
{
    switch (mirrorAxis)
    {
    case EAxis::X:
        value.X *= -1.0;
        break;
    case EAxis::Z:
        value.Z *= -1.0;
        break;
    case EAxis::Y:
    default:
        value.Y *= -1.0;
        break;
    }
    return value;
}

USkeletalMeshComponent* findInteractionPointMesh(
    UBlueprint* blueprint,
    USCS_Node* pointNode,
    const FName rightBone,
    const FName leftBone)
{
    auto supportsBones = [rightBone, leftBone](
        const USkeletalMeshComponent* mesh)
    {
        const USkeletalMesh* asset = mesh != nullptr
            ? mesh->GetSkeletalMeshAsset()
            : nullptr;
        return asset != nullptr &&
            asset->GetRefSkeleton().FindBoneIndex(rightBone) != INDEX_NONE &&
            asset->GetRefSkeleton().FindBoneIndex(leftBone) != INDEX_NONE;
    };

    USkeletalMeshComponent* parentMesh = pointNode != nullptr
        ? Cast<USkeletalMeshComponent>(
            pointNode->GetParentComponentTemplate(blueprint))
        : nullptr;
    if (supportsBones(parentMesh))
    {
        return parentMesh;
    }

    if (blueprint != nullptr && blueprint->SimpleConstructionScript != nullptr)
    {
        for (USCS_Node* node :
             blueprint->SimpleConstructionScript->GetAllNodes())
        {
            USkeletalMeshComponent* mesh =
                Cast<USkeletalMeshComponent>(node->ComponentTemplate);
            if (supportsBones(mesh))
            {
                return mesh;
            }
        }
    }

    AActor* defaultActor =
        blueprint != nullptr && blueprint->GeneratedClass != nullptr
            ? Cast<AActor>(blueprint->GeneratedClass->GetDefaultObject())
            : nullptr;
    if (defaultActor != nullptr)
    {
        TArray<USkeletalMeshComponent*> meshes;
        defaultActor->GetComponents(meshes);
        for (USkeletalMeshComponent* mesh : meshes)
        {
            if (supportsBones(mesh))
            {
                return mesh;
            }
        }
    }
    return nullptr;
}

bool showInteractionPointMirrorAxisDialog(EAxis::Type& outAxis)
{
    TArray<TSharedPtr<FString>> axisOptions = {
        MakeShared<FString>(TEXT("X")),
        MakeShared<FString>(TEXT("Y")),
        MakeShared<FString>(TEXT("Z"))};
    TSharedPtr<FString> selectedAxis = axisOptions[1];
    bool accepted = false;
    TSharedRef<SWindow> window = SNew(SWindow)
        .Title(LOCTEXT(
            "MirrorInteractionPointsTitle",
            "Copy Semantic Interaction Points Right to Left"))
        .ClientSize(FVector2D(390.0, 135.0))
        .SupportsMaximize(false)
        .SupportsMinimize(false);

    window->SetContent(
        SNew(SBorder)
        .Padding(12.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT(
                    "MirrorInteractionPointsAxisLabel",
                    "Reference-mesh axis whose coordinate should be negated"))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&axisOptions)
                .InitiallySelectedItem(selectedAxis)
                .OnGenerateWidget_Lambda(
                    [](const TSharedPtr<FString>& option)
                    {
                        return SNew(STextBlock)
                            .Text(FText::FromString(*option));
                    })
                .OnSelectionChanged_Lambda(
                    [&selectedAxis](
                        const TSharedPtr<FString>& option,
                        ESelectInfo::Type)
                    {
                        selectedAxis = option;
                    })
                [
                    SNew(STextBlock)
                    .Text_Lambda(
                        [&selectedAxis]()
                        {
                            return selectedAxis.IsValid()
                                ? FText::FromString(*selectedAxis)
                                : FText::GetEmpty();
                        })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("CancelInteractionPointMirror", "Cancel"))
                    .OnClicked_Lambda(
                        [window]()
                        {
                            window->RequestDestroyWindow();
                            return FReply::Handled();
                        })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ApplyInteractionPointMirror", "Copy"))
                    .OnClicked_Lambda(
                        [&accepted, window]()
                        {
                            accepted = true;
                            window->RequestDestroyWindow();
                            return FReply::Handled();
                        })
                ]
            ]
        ]);

    GEditor->EditorAddModalWindow(window);
    if (!accepted || !selectedAxis.IsValid())
    {
        return false;
    }

    outAxis = *selectedAxis == TEXT("X")
        ? EAxis::X
        : *selectedAxis == TEXT("Z")
            ? EAxis::Z
            : EAxis::Y;
    return true;
}

void synchronizeInteractionPoints(
    USemanticInteractionPointsComponent* component)
{
    UBlueprint* blueprint = getInteractionPointsOwningBlueprint(component);
    if (component == nullptr ||
        blueprint == nullptr ||
        blueprint->SimpleConstructionScript == nullptr)
    {
        return;
    }

    TArray<USemanticInteractionPointComponent*> points;
    bool categoryChanged = false;
    const FText category = LOCTEXT(
        "SemanticInteractionPointCategory",
        "Semantic Interaction Points");
    for (USCS_Node* node : blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (USemanticInteractionPointComponent* point =
            Cast<USemanticInteractionPointComponent>(node->ComponentTemplate))
        {
            points.Add(point);
            if (!node->CategoryName.EqualTo(category))
            {
                node->CategoryName = category;
                categoryChanged = true;
            }
        }
        else if (Cast<USemanticInteractionPointsComponent>(
                     node->ComponentTemplate) != nullptr &&
                 !node->CategoryName.EqualTo(category))
        {
            node->CategoryName = category;
            categoryChanged = true;
        }
    }

    bool referencesChanged = component->interactionPoints.Num() != points.Num();
    if (!referencesChanged)
    {
        for (int32 index = 0; index < points.Num(); ++index)
        {
            if (component->interactionPoints[index] != points[index])
            {
                referencesChanged = true;
                break;
            }
        }
    }
    if (referencesChanged)
    {
        component->Modify();
        component->interactionPoints.Reset(points.Num());
        for (USemanticInteractionPointComponent* point : points)
        {
            component->interactionPoints.Add(point);
        }
        component->PostEditChange();
        component->MarkPackageDirty();
    }
    if (referencesChanged || categoryChanged)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(blueprint);
    }
}
}

TSharedRef<IDetailCustomization>
FSemanticInteractionPointsComponentCustomization::makeInstance()
{
    return MakeShared<FSemanticInteractionPointsComponentCustomization>();
}

void FSemanticInteractionPointsComponentCustomization::CustomizeDetails(
    IDetailLayoutBuilder& detailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> objects;
    detailBuilder.GetObjectsBeingCustomized(objects);
    component = objects.IsEmpty()
        ? nullptr
        : Cast<USemanticInteractionPointsComponent>(objects[0].Get());

    synchronizeInteractionPoints(component.Get());

    IDetailCategoryBuilder& category =
        detailBuilder.EditCategory(TEXT("Semantic Interaction Points"));
    category.AddCustomRow(LOCTEXT(
        "CopyInteractionPointsSearch",
        "Copy Right to Left"))
        .WholeRowContent()
        [
            SNew(SButton)
            .Text(LOCTEXT("CopyInteractionPointsRightToLeft", "Copy Right to Left"))
            .ToolTipText(LOCTEXT(
                "CopyInteractionPointsRightToLeftTooltip",
                "Create or update left interaction points from right interaction points using reference-pose bone transforms."))
            .OnClicked(
                this,
                &FSemanticInteractionPointsComponentCustomization::copyRightToLeft)
        ];
}

FReply FSemanticInteractionPointsComponentCustomization::copyRightToLeft()
{
    USemanticInteractionPointsComponent* interactionPoints = component.Get();
    UBlueprint* blueprint = getInteractionPointsOwningBlueprint(
        interactionPoints);
    if (interactionPoints == nullptr ||
        blueprint == nullptr ||
        blueprint->SimpleConstructionScript == nullptr)
    {
        return FReply::Handled();
    }

    EAxis::Type mirrorAxis = EAxis::Y;
    if (!showInteractionPointMirrorAxisDialog(mirrorAxis))
    {
        return FReply::Handled();
    }

    USimpleConstructionScript* constructionScript =
        blueprint->SimpleConstructionScript;
    TMap<FName, USCS_Node*> nodesByPointId;
    TArray<USCS_Node*> rightNodes;
    for (USCS_Node* node : constructionScript->GetAllNodes())
    {
        if (USemanticInteractionPointComponent* point =
            Cast<USemanticInteractionPointComponent>(node->ComponentTemplate))
        {
            nodesByPointId.Add(point->pointId, node);
            FString pointId = point->pointId.ToString();
            if (pointId.StartsWith(TEXT("right"), ESearchCase::IgnoreCase))
            {
                rightNodes.Add(node);
            }
        }
    }

    const FScopedTransaction transaction(LOCTEXT(
        "CopyInteractionPointsTransaction",
        "Copy Semantic Interaction Points Right to Left"));
    blueprint->Modify();
    constructionScript->Modify();

    int32 copiedCount = 0;
    int32 skippedCount = 0;
    TMap<FString, int32> skipReasons;
    auto recordSkip = [&skippedCount, &skipReasons](const FString& reason)
    {
        ++skippedCount;
        ++skipReasons.FindOrAdd(reason);
    };
    for (USCS_Node* rightNode : rightNodes)
    {
        USemanticInteractionPointComponent* rightPoint =
            Cast<USemanticInteractionPointComponent>(
                rightNode->ComponentTemplate);
        if (rightPoint == nullptr || rightNode->AttachToName.IsNone())
        {
            recordSkip(TEXT("missing point component or socket"));
            continue;
        }

        FString leftPointIdString = rightPoint->pointId.ToString();
        FString leftGroupIdString = rightPoint->interactionGroupId.ToString();
        FString leftBoneString = rightNode->AttachToName.ToString();
        if (!replaceRightWithLeft(leftPointIdString) ||
            !replaceRightWithLeft(leftGroupIdString) ||
            !replaceRightWithLeft(leftBoneString))
        {
            recordSkip(TEXT("right-side name could not be converted"));
            continue;
        }

        const FName leftPointId(*leftPointIdString);
        const FName leftBone(*leftBoneString);
        USkeletalMeshComponent* mesh = findInteractionPointMesh(
            blueprint,
            rightNode,
            rightNode->AttachToName,
            leftBone);
        if (mesh == nullptr || mesh->GetSkeletalMeshAsset() == nullptr)
        {
            recordSkip(TEXT("skeletal mesh parent could not be resolved"));
            continue;
        }

        const FReferenceSkeleton& skeleton =
            mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
        const int32 rightBoneIndex = skeleton.FindBoneIndex(
            rightNode->AttachToName);
        const int32 leftBoneIndex = skeleton.FindBoneIndex(leftBone);
        const TArray<FTransform> boneTransforms =
            getInteractionPointReferenceBoneTransforms(skeleton);
        if (!boneTransforms.IsValidIndex(rightBoneIndex) ||
            !boneTransforms.IsValidIndex(leftBoneIndex))
        {
            recordSkip(TEXT("matching left socket was not found"));
            continue;
        }

        USCS_Node* leftNode = nodesByPointId.FindRef(leftPointId);
        if (leftNode == nullptr)
        {
            USCS_Node* meshNode = nullptr;
            for (USCS_Node* candidate : constructionScript->GetAllNodes())
            {
                if (candidate->ComponentTemplate == mesh)
                {
                    meshNode = candidate;
                    break;
                }
            }
            FString variableName = rightNode->GetVariableName().ToString();
            if (!replaceRightWithLeft(variableName))
            {
                variableName = FString::Printf(
                    TEXT("SemanticInteraction_%s"),
                    *leftPointIdString);
            }
            leftNode = constructionScript->CreateNode(
                USemanticInteractionPointComponent::StaticClass(),
                FName(*variableName));
            if (meshNode != nullptr)
            {
                meshNode->AddChildNode(leftNode);
            }
            else
            {
                constructionScript->AddNode(leftNode);
                leftNode->SetParent(mesh);
            }
            nodesByPointId.Add(leftPointId, leftNode);
        }

        USemanticInteractionPointComponent* leftPoint =
            Cast<USemanticInteractionPointComponent>(
                leftNode->ComponentTemplate);
        if (leftPoint == nullptr)
        {
            recordSkip(TEXT("left component could not be created"));
            continue;
        }

        const FTransform& rightBoneTransform =
            boneTransforms[rightBoneIndex];
        const FTransform& leftBoneTransform =
            boneTransforms[leftBoneIndex];
        const FQuat rightRotation = rightPoint->GetRelativeRotation()
            .Quaternion();
        const FVector mirroredMeshLocation = mirrorVector(
            rightBoneTransform.TransformPosition(
                rightPoint->GetRelativeLocation()),
            mirrorAxis);
        const FVector mirroredMeshForward = mirrorVector(
            rightBoneTransform.TransformVectorNoScale(
                rightRotation.RotateVector(FVector::ForwardVector)),
            mirrorAxis);
        const FVector mirroredMeshUp = mirrorVector(
            rightBoneTransform.TransformVectorNoScale(
                rightRotation.RotateVector(FVector::UpVector)),
            mirrorAxis);

        const FVector leftLocation = leftBoneTransform
            .InverseTransformPosition(mirroredMeshLocation);
        const FVector leftForward = leftBoneTransform
            .InverseTransformVectorNoScale(mirroredMeshForward)
            .GetSafeNormal();
        const FVector leftUp = leftBoneTransform
            .InverseTransformVectorNoScale(mirroredMeshUp)
            .GetSafeNormal();
        const FQuat leftRotation = FRotationMatrix::MakeFromXZ(
            leftForward,
            leftUp).ToQuat();

        leftPoint->Modify();
        leftPoint->pointId = leftPointId;
        leftPoint->interactionGroupId = FName(*leftGroupIdString);
        leftPoint->pointRadius = rightPoint->pointRadius;
        leftPoint->enabled = rightPoint->enabled;
        leftPoint->SetRelativeLocation(leftLocation);
        leftPoint->SetRelativeRotation(leftRotation);
        leftPoint->SetRelativeScale3D(rightPoint->GetRelativeScale3D());
        leftNode->AttachToName = leftBone;
        ++copiedCount;
    }

    synchronizeInteractionPoints(interactionPoints);
    if (copiedCount > 0)
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
        FKismetEditorUtilities::CompileBlueprint(blueprint);
        SurfacePatchEditorRefresh::scheduleBlueprintPreviewRefresh(blueprint);
    }

    FString result = FString::Printf(
        TEXT("Copied %d right interaction point(s) to the left. Skipped %d."),
        copiedCount,
        skippedCount);
    for (const TPair<FString, int32>& reason : skipReasons)
    {
        result += FString::Printf(
            TEXT("\n%d: %s"),
            reason.Value,
            *reason.Key);
    }
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(result));
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
