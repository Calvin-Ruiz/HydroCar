#pragma once

#include "CoreMinimal.h"
#include "FSaveData.generated.h"

class SaveData;

USTRUCT(BlueprintType)
struct FSaveData {
    GENERATED_BODY()
public:
    SaveData *handle = nullptr;
};
