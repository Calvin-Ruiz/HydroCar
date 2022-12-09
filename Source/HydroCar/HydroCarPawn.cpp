// Fill out your copyright notice in the Description page of Project Settings.


#include "HydroCarPawn.h"
#include "Interactive.h"

void AHydroCarPawn::NotifyActorBeginOverlap(AActor* other)
{
	if (auto *ptr = Cast<IInteractive>(other)) {
		ptr->Execute_OnNear(other, this);
	}
}

void AHydroCarPawn::NotifyActorEndOverlap(AActor* other)
{
	if (auto* ptr = Cast<IInteractive>(other)) {
		ptr->Execute_OnFar(other, this);
	}
}
