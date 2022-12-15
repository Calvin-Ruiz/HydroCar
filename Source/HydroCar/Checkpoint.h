// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "Interactive.h"
#include "Checkpoint.generated.h"

/**
 *
 */
UCLASS()
class HYDROCAR_API ACheckpoint : public ATriggerBox, public IInteractive
{
	GENERATED_BODY()

public:
	virtual void OnNear_Implementation(AHydroCarPawn *player) override;
	virtual void OnFar_Implementation(AHydroCarPawn *player) override;

protected:
	UPROPERTY(Category = Race, EditAnywhere, BlueprintReadOnly)
	int checkPointNumber = 0;
};
