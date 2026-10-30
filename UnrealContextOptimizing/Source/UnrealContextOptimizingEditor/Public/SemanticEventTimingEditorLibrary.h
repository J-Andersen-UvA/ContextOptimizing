#pragma once

#include "CoreMinimal.h"

class UAnimSequenceBase;

enum class ESemanticZeroType : uint8
{
    Approach,
    Recovery
};

class UNREALCONTEXTOPTIMIZINGEDITOR_API FSemanticZeroCalculator
{
public:
    static bool calculateSemanticZeroFrame(
        const UAnimSequenceBase* animation,
        FName boneName,
        int32 representativeFrame,
        int32 eventBoundaryFrame,
        ESemanticZeroType zeroType,
        int32& zeroFrame,
        FString& errorMessage,
        int32* holdEndFrame = nullptr);
};
