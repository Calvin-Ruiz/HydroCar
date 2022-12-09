// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Tools/BigSave.hpp"
#include "HydroCarGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class HYDROCAR_API AHydroCarGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;

private:
	std::shared_ptr<BigSave> save;
};
