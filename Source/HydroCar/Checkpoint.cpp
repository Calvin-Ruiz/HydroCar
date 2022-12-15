// Fill out your copyright notice in the Description page of Project Settings.

#include "Checkpoint.h"
#include "HydroCarPawn.h"

void ACheckpoint::OnNear_Implementation(AHydroCarPawn *player)
{
    player->reachCheckpoint(checkPointNumber);
}

void ACheckpoint::OnFar_Implementation(AHydroCarPawn *player)
{
}
