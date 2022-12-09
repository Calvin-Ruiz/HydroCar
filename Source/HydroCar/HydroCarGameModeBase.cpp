// Copyright Epic Games, Inc. All Rights Reserved.


#include "HydroCarGameModeBase.h"


void AHydroCarGameModeBase::StartPlay()
{
	save = BigSave::loadShared("GameSave");
}
