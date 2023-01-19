#pragma once

#include "CoreMinimal.h"
#include "FSaveData.generated.h"

class SaveData;

USTRUCT(BlueprintType, NoExport, meta = (DontUseGenericSpawnObject))
struct FSaveData {
public:
    SaveData *handle = nullptr;
};
