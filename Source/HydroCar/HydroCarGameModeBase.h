// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Tools/BigSave.hpp"
#include "HydroCarGameModeBase.generated.h"

class AHydroCarPawn;

/**
 *
 */
UCLASS()
class HYDROCAR_API AHydroCarGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	static AHydroCarGameModeBase *instance;

	virtual void StartPlay() override;

	void respawn(AHydroCarPawn *self);

private:
	std::shared_ptr<BigSave> save;
};
