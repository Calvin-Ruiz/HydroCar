// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "Tools/SaveData.hpp"
#include "HydroCarPawn.generated.h"

/**
 *
 */
UCLASS()
class HYDROCAR_API AHydroCarPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	AHydroCarPawn();
	virtual void NotifyActorBeginOverlap(AActor *other) override;
	virtual void NotifyActorEndOverlap(AActor* other) override;

	virtual void Tick(float DeltaTime) override; // Function that is called every frame

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Throttle + Steering
	void ApplyThrottle(float Val); // Val = how much force we want to apply
	void ApplySteering(float Val);

	// Camera mouse input
	void Lookup(float Val);
	void Turn(float Val);

	// Handbrake
	void OnHandbrakePressed();
	void OnHandbrakeReleased();

	// Respawn
	// void setRespawnLocation(FVector LocToSave);
	// FVector getRespawnLocation();

	// Lap
	// void increaseLap();
	// UFUNCTION(Category = Race, BlueprintCallable, BlueprintPure)
	// int getCurrentLap();
	// UPROPERTY(Category = Race, EditAnywhere, BlueprintReadOnly)
	// int maxCurrentLap = 3;

	// EndGame + Timer
	void endGame();
	void startTimer();

	UPROPERTY(Category = Timer, EditDefaultsOnly, BlueprintReadOnly)
	int seconds;

	// Tire variables
	UPROPERTY(Category = Tire, EditDefaultsOnly, BlueprintReadOnly)
	float MinTireLoad = 0.0f;
	UPROPERTY(Category = Tire, EditDefaultsOnly, BlueprintReadOnly)
	float MinTireLoadFiltered = 0.2f;
	UPROPERTY(Category = Tire, EditDefaultsOnly, BlueprintReadOnly)
	float MaxTireLoad = 2.0f;
	UPROPERTY(Category = Tire, EditDefaultsOnly, BlueprintReadOnly)
	float MaxTireLoadFiltered = 2.0f;

	// Torque variables
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float MaxEngineRPM = 5700.0f;
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float FirstTorqueMin = 0.0f;
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float FirstTorqueMax = 400.0f;
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float SecondTorqueMin = 1890.0f;
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float SecondTorqueMax = 500.0f;
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float ThirdTorqueMin = 5730.0f;
	UPROPERTY(Category = Torque, EditDefaultsOnly, BlueprintReadOnly)
	float ThirdTorqueMax = 400.0f;

	// Steering variables
	UPROPERTY(Category = Sterring, EditDefaultsOnly, BlueprintReadOnly)
	float FirstSteeringMin = 0.0f;
	UPROPERTY(Category = Sterring, EditDefaultsOnly, BlueprintReadOnly)
	float FirstSteeringMax = 1.0f;
	UPROPERTY(Category = Sterring, EditDefaultsOnly, BlueprintReadOnly)
	float SecondSteeringMin = 40.0f;
	UPROPERTY(Category = Sterring, EditDefaultsOnly, BlueprintReadOnly)
	float SecondSteeringMax = 0.7f;
	UPROPERTY(Category = Sterring, EditDefaultsOnly, BlueprintReadOnly)
	float ThirdSteeringMin = 120.0f;
	UPROPERTY(Category = Sterring, EditDefaultsOnly, BlueprintReadOnly)
	float ThirdSteeringMax = 0.6f;

	// Gearbox variables
	UPROPERTY(Category = Gearbox, EditDefaultsOnly, BlueprintReadOnly)
	bool GearAutoBox = true;
	UPROPERTY(Category = Gearbox, EditDefaultsOnly, BlueprintReadOnly)
	float GearSwitchTime = 0.15f;
	UPROPERTY(Category = Gearbox, EditDefaultsOnly, BlueprintReadOnly)
	float GearAutoBoxLatency = 1.0f;

	// Air Physics
	//void UpdateInAirControl(float DeltaTime);

protected:

	// Spring arm for the camera
	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* SpringArm;

	// Camera
	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* Camera;

protected:
	// Save every information about the player
	// UFUNCTION()
	// SaveData SaveCurrentState();

	// Load every information about the player
	// UFUNCTION()
	// void LoadCurrentState(SaveData sd);

	UPROPERTY()
	FString playerName;
};
