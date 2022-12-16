// Copyright Epic Games, Inc. All Rights Reserved.


#include "HydroCarGameModeBase.h"
#include "HydroCarPawn.h"

AHydroCarGameModeBase *AHydroCarGameModeBase::instance = nullptr;

void AHydroCarGameModeBase::StartPlay()
{
	instance = this;
	save = BigSave::loadShared("GameSave");
	Super::StartPlay();
}

void AHydroCarGameModeBase::respawn(AHydroCarPawn *self)
{
	RestartPlayer(self->GetController());
}
