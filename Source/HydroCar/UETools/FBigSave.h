#pragma once

#include "CoreMinimal.h"
#include "FSaveData.h"
#include "../Tools/BigSave.hpp"
#include <memory>
#include "FBigSave.generated.h"

USTRUCT(BlueprintType, Atomic, meta = (DontUseGenericSpawnObject))
struct FBigSave {
    GENERATED_BODY();
public:
	std::shared_ptr<BigSave> base;
	UPROPERTY(BlueprintReadOnly)
	FSaveData root = FSaveData{base.get()};
};
