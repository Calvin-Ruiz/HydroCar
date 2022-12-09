// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "HydroCarPawn.generated.h"

/**
 * 
 */
UCLASS()
class HYDROCAR_API AHydroCarPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	virtual void NotifyActorBeginOverlap(AActor *other) override;
	virtual void NotifyActorEndOverlap(AActor* other) override;
};
