// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "Tools/BigSave.hpp"
#include "UETools/FSaveData.h"
#include <chrono>
#include "HydroCarGameModeBase.h"
#include "HydroCarPawn.generated.h"

class UBaseWidget;
class UConfirmationWidget;

// Enumerate all sections at the root of a player save
UENUM(BlueprintType)
enum Section {
	S_CHECKPOINTS,
	S_ACHIEVEMENTS, // List of int describing the progression
	S_STATISTICS,
	S_SETTINGS,
	S_MARKET,
	// Only add new sections at the end of this enum, right above this comment, to maintain save compatibility
};

USTRUCT(BlueprintType)
struct FAchievementDisplay {
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	FString name;
	UPROPERTY(BlueprintReadOnly)
	FString description;
	UPROPERTY(BlueprintReadOnly)
	int progression = 0;
	UPROPERTY(BlueprintReadOnly)
	int completion = 1;
	UPROPERTY(BlueprintReadOnly)
	int ucCompletion = 0;
};

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

	virtual void BecomeViewTarget(APlayerController *PC) override;
	virtual void EndViewTarget(APlayerController *PC) override;

	virtual void Tick(float deltaTime) override; // Function that is called every frame

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Throttle + Steering
	void ApplyThrottle(float val); // val = how much force we want to apply
	void ApplySteering(float val);

	// Camera mouse input
	void Lookup(float val);
	void Turn(float val);

	// Handbrake
	void OnHandbrakePressed();
	void OnHandbrakeReleased();

	UFUNCTION(BlueprintCallable)
	void dropCheckpoint();
	UFUNCTION(BlueprintCallable)
	void saveCheckpoint();
	void saveCheckpointInternal();
	UFUNCTION(BlueprintCallable)
	void loadCheckpoint();
	void loadCheckpointInternal();
	void endGame();
	void OnBack();

	void OnLeftThrustTrue() {
		OnLeftThrust(true);
		leftThrust = true;
		UsingHydrogen = true;
	}
	void OnRightThrustTrue() {
		OnRightThrust(true);
		rightThrust = true;
		UsingHydrogen = true;
	}
	void OnBoostTrue() {
		OnBackThrust(true);
		backThrust = true;
		UsingHydrogen = true;
	}
	void OnLeftThrustFalse() {
		if (leftThrust) {
			OnLeftThrust(false);
			leftThrust = false;
			UsingHydrogen &= rightThrust | backThrust;
		}
	}
	void OnRightThrustFalse() {
		if (rightThrust) {
			OnRightThrust(false);
			rightThrust = false;
			UsingHydrogen &= leftThrust | backThrust;
		}
	}
	void OnBoostFalse() {
		if (backThrust) {
			OnBackThrust(false);
			backThrust = false;
			UsingHydrogen &= leftThrust | rightThrust;
		}
	}

	UFUNCTION(BlueprintCallable)
	void loadConfig(const FString &playerName);

	UFUNCTION(BlueprintCallable)
	void onBegin();

	UFUNCTION(BlueprintCallable)
	void onRestart();

	UFUNCTION(BlueprintCallable)
	void pauseToMainMenu();

	UFUNCTION(BlueprintImplementableEvent)
	void OnBeginGame();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEndGame();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateRPM(float EngineRPM);

	// Called when completing an achievement
	UFUNCTION(BlueprintImplementableEvent)
	void OnAchievement(FAchievementDisplay achievement, bool isNew);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSaveCheckpoint(FSaveData datas);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadCheckpoint(FSaveData datas);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLeftThrust(bool state);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRightThrust(bool state);

	UFUNCTION(BlueprintImplementableEvent)
	void OnBackThrust(bool state);

	void stopThrusters();

	// Increase the counter of the given achievement
	UFUNCTION(BlueprintCallable)
	void updateAchievement(AchievementName name);

	UFUNCTION(BlueprintCallable)
	TArray<FAchievementDisplay> queryAchievements();

	UPROPERTY(Category = Timer, VisibleInstanceOnly, BlueprintReadOnly)
	FString timer = "Timer: 00:00.000";

	UPROPERTY(Category = Timer, VisibleInstanceOnly, BlueprintReadOnly)
	FString bestTime = "Best: 00:00.000";

	// Number of unique checkpoint in a Lap
	UPROPERTY(Category = Game, EditDefaultsOnly, BlueprintReadOnly)
	int checkPointCount = 1;
	// Number of lap required
	UPROPERTY(Category = Game, EditDefaultsOnly, BlueprintReadOnly)
	int targetLapCount = 3;

	int prevCheckPoint;
	int nextCheckPoint;
	UPROPERTY(Category = Game, EditDefaultsOnly, BlueprintReadOnly)
	int currentLap = 0;

	UPROPERTY(Category = Menu, EditDefaultsOnly, meta = (DisplayName = "Overlay"))
	TSubclassOf<UBaseWidget> overlay;
	UPROPERTY(Category = Menu, EditDefaultsOnly, meta = (DisplayName = "Main Menu"))
	TSubclassOf<UBaseWidget> mainMenuClass;
	UPROPERTY(Category = Menu, EditDefaultsOnly, meta = (DisplayName = "Pause Menu"))
	TSubclassOf<UBaseWidget> pauseMenuClass;
	UPROPERTY(Category = Menu, EditDefaultsOnly, meta = (DisplayName = "Confirmation"))
	TSubclassOf<UConfirmationWidget> confirmationClass;
	UPROPERTY(Category = Menu, BlueprintReadOnly)
	UBaseWidget *display = nullptr;
	UPROPERTY(Category = Menu, BlueprintReadOnly)
	UBaseWidget *mainMenu = nullptr;
	UPROPERTY()
	UBaseWidget *pauseMenu = nullptr;
	UPROPERTY()
	UConfirmationWidget *confirmation = nullptr;

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
	float AngleRatio = 1.0f;
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
	bool AutoReverse = true;
	UPROPERTY(Category = Gearbox, EditDefaultsOnly, BlueprintReadOnly)
	float GearSwitchTime = 0.15f;
	UPROPERTY(Category = Gearbox, EditDefaultsOnly, BlueprintReadOnly)
	float GearAutoBoxLatency = 1.0f;

	// Amount of hydrogen available
	UPROPERTY(Category = Hydrogen, BlueprintReadOnly)
	float Hydrogen;
	UPROPERTY(Category = Hydrogen, EditDefaultsOnly, BlueprintReadOnly)
	float HydrogenCapacity = 10.f;
	UPROPERTY(Category = Hydrogen, EditDefaultsOnly, BlueprintReadOnly)
	float HydrogenRenegenation = 0.f;
	// Propulsion force and consumption of the hydrogen thruster
	UPROPERTY(Category = Hydrogen, EditDefaultsOnly, BlueprintReadOnly)
	float HydrogenThruster = 2.f;
	UPROPERTY(Category = Hydrogen, BlueprintReadOnly)
	bool UsingHydrogen = false;

	// Air Physics
	//void UpdateInAirControl(float DeltaTime);

	// Return true if it was the expected checkpoint
	bool reachCheckpoint(int checkPointNumber);

	// Cumulated control of all widgets
	int8 controlDependency = 0;
	inline void setNewInputTarget(UBaseWidget *_newInputTarget) {
		// UE_LOG(LogTemp, Warning, TEXT("Set New Input Target"));
		newInputTarget = _newInputTarget;
	}

	void applyControl();
	void updateControl(int8 newControl);
	BigSave saved;
	UPROPERTY(AdvancedDisplay, NoClear, BlueprintReadOnly)
	FSaveData Saved = {&saved};
	UPROPERTY(AdvancedDisplay, NoClear, BlueprintReadOnly)
	FSaveData ConfirmationSkip;

protected:
	// Spring arm for the camera
	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;

	// Camera
	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float viewRotationSpeed = 1;

private:
	void setDirection(int32 direction);
	int32 cachedDirection = 0;
	UBaseWidget *newInputTarget = nullptr;
	float lastRPM;
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::duration pauseRememberance;
	bool started = false;
public:
	bool leftThrust = false;
	bool rightThrust = false;
	bool backThrust = false;
};
