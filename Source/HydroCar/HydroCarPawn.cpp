// Fill out your copyright notice in the Description page of Project Settings.


#include "HydroCarPawn.h"
#include "Interactive.h"
#include "HydroCarGameModeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Tools/BigSave.hpp"

AHydroCarPawn::AHydroCarPawn()
{
	UChaosWheeledVehicleMovementComponent* Vehicle4W = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	// Adjust the tire
	//Vehicle4W->MinNormalizedTireLoad = MinTireLoad;
	//Vehicle4W->MinNormalizedTireLoadFiltered = MinTireLoadFiltered;
	//Vehicle4W->MaxNormalizedTireLoad = MaxTireLoad;
	//Vehicle4W->MaxNormalizedTireLoadFiltered = MaxTireLoadFiltered;

	// Torque setup
	Vehicle4W->EngineSetup.MaxRPM = MaxEngineRPM;
	Vehicle4W->EngineSetup.TorqueCurve.GetRichCurve()->Reset();
	Vehicle4W->EngineSetup.TorqueCurve.GetRichCurve()->AddKey(FirstTorqueMin, FirstTorqueMax);
	Vehicle4W->EngineSetup.TorqueCurve.GetRichCurve()->AddKey(SecondTorqueMin, SecondTorqueMax);
	Vehicle4W->EngineSetup.TorqueCurve.GetRichCurve()->AddKey(ThirdTorqueMin, ThirdTorqueMax);

	// Adjust the steering
	Vehicle4W->SteeringSetup.AngleRatio = AngleRatio;
	Vehicle4W->SteeringSetup.SteeringCurve.GetRichCurve()->Reset();
	Vehicle4W->SteeringSetup.SteeringCurve.GetRichCurve()->AddKey(FirstSteeringMin, FirstSteeringMax);
	Vehicle4W->SteeringSetup.SteeringCurve.GetRichCurve()->AddKey(SecondSteeringMin, SecondSteeringMax);
	Vehicle4W->SteeringSetup.SteeringCurve.GetRichCurve()->AddKey(ThirdSteeringMin, ThirdSteeringMax);
	Vehicle4W->SteeringSetup.SteeringType = ESteeringType::Ackermann;

	Vehicle4W->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
	Vehicle4W->DifferentialSetup.FrontRearSplit = 0.65f;

	// Automatic gearbox
	Vehicle4W->TransmissionSetup.bUseAutomaticGears = GearAutoBox;
	Vehicle4W->TransmissionSetup.bUseAutoReverse = AutoReverse;
	Vehicle4W->TransmissionSetup.GearChangeTime = GearSwitchTime;
	Vehicle4W->TransmissionSetup.ChangeDownRPM = GearAutoBoxLatency;
	Vehicle4W->TransmissionSetup.ChangeUpRPM = GearAutoBoxLatency;

	// Spring arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 250.0f;
	SpringArm->bUsePawnControlRotation = true;

	// Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->FieldOfView = 90.0f;
}

void AHydroCarPawn::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (started) {
		int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
		timer[15] = '0' + milliseconds % 10;
		milliseconds /= 10;
		timer[14] = '0' + milliseconds % 10;
		milliseconds /= 10;
		timer[13] = '0' + milliseconds % 10;
		milliseconds /= 10;
		timer[11] = '0' + milliseconds % 10;
		milliseconds /= 10;
		timer[10] = '0' + milliseconds % 6;
		milliseconds /= 6;
		timer[8] = '0' + milliseconds % 10;
		milliseconds /= 10;
		timer[7] = '0' + milliseconds % 10;
	}
	//UpdateInAirControl(DeltaTime);
}

void AHydroCarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Throttle", this, &AHydroCarPawn::ApplyThrottle);
	PlayerInputComponent->BindAxis("Steer", this, &AHydroCarPawn::ApplySteering);
	PlayerInputComponent->BindAxis("LookUp", this, &AHydroCarPawn::Lookup);
	PlayerInputComponent->BindAxis("Turn", this, &AHydroCarPawn::Turn);

	PlayerInputComponent->BindAction("Handbrake", IE_Pressed, this, &AHydroCarPawn::OnHandbrakePressed);
	PlayerInputComponent->BindAction("Handbrake", IE_Released, this, &AHydroCarPawn::OnHandbrakeReleased);
	PlayerInputComponent->BindAction("Respawn", IE_Released, this, &AHydroCarPawn::loadCheckpoint);
	PlayerInputComponent->BindAction("DropCheckpoint", IE_Pressed, this, &AHydroCarPawn::dropCheckpoint);
	PlayerInputComponent->BindAction("DropCheckpoint", IE_Released, this, &AHydroCarPawn::loadCheckpoint);
}

void AHydroCarPawn::setDirection(int32 direction)
{
	if (direction != cachedDirection) {
		cachedDirection = direction;
		GetVehicleMovementComponent()->SetTargetGear(direction, true);
	}
}

void AHydroCarPawn::ApplyThrottle(float val)
{
	if (val < 0) {
		val = -val;
		setDirection(-1);
	} else
		setDirection(1);
	GetVehicleMovementComponent()->SetThrottleInput(val);
}

void AHydroCarPawn::ApplySteering(float val)
{
	GetVehicleMovementComponent()->SetSteeringInput(val);
}

void AHydroCarPawn::Lookup(float val)
{
	if (val != 0.f) {
		AddControllerPitchInput(val);
	}
}

void AHydroCarPawn::Turn(float val)
{
	if (val != 0.f) {
		AddControllerYawInput(val);
	}
}

void AHydroCarPawn::OnHandbrakePressed()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void AHydroCarPawn::OnHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

void AHydroCarPawn::NotifyActorBeginOverlap(AActor* other)
{
	if (auto *ptr = Cast<IInteractive>(other)) {
		ptr->Execute_OnNear(other, this);
	}
}

void AHydroCarPawn::NotifyActorEndOverlap(AActor* other)
{
	if (auto *ptr = Cast<IInteractive>(other)) {
		ptr->Execute_OnFar(other, this);
	}
}

bool AHydroCarPawn::reachCheckpoint(int checkPointNumber)
{
	if (checkPointNumber == nextCheckPoint) {
		if (!started) {
			startTime = std::chrono::steady_clock::now();
			started = true;
		}
		prevCheckPoint = (checkPointNumber + checkPointCount - 1) % checkPointCount;
		if (++nextCheckPoint == checkPointCount) {
			nextCheckPoint = 0;
			if (currentLap++ == targetLapCount) {
				endGame();
				return true;
			}
		}
		saveCheckpoint();
		return true;
	} else if (checkPointNumber == prevCheckPoint) {
		if (!started) {
			currentLap = -1;
			startTime = std::chrono::steady_clock::now();
			started = true;
		}
		nextCheckPoint = (checkPointNumber + 1) % checkPointCount;
		if (--prevCheckPoint < 0) {
			prevCheckPoint += checkPointCount;
			if (currentLap-- == -targetLapCount) {
				endGame();
				return true;
			}
		}
		saveCheckpoint();
		return true;
	}
	return false;
}

void AHydroCarPawn::onBegin()
{
	currentLap = 1;
	nextCheckPoint = 0;
	prevCheckPoint = checkPointCount - 1;
	if (saved[S_CHECKPOINTS].empty())
		return;
	int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(saved[S_CHECKPOINTS].get<std::chrono::steady_clock::duration>()).count();
	bestTime[14] = '0' + milliseconds % 10;
	milliseconds /= 10;
	bestTime[13] = '0' + milliseconds % 10;
	milliseconds /= 10;
	bestTime[12] = '0' + milliseconds % 10;
	milliseconds /= 10;
	bestTime[10] = '0' + milliseconds % 10;
	milliseconds /= 10;
	bestTime[9] = '0' + milliseconds % 6;
	milliseconds /= 6;
	bestTime[7] = '0' + milliseconds % 10;
	milliseconds /= 10;
	bestTime[6] = '0' + milliseconds % 10;
	if (saved[S_CHECKPOINTS].size()) {
		loadCheckpoint();
		--saved[S_STATISTICS]["checkpoint"]["loaded"].get<int>();
		started = true;
	}
}

void AHydroCarPawn::endGame()
{
	// Work In Progress
	auto duration = std::chrono::steady_clock::now() - startTime;
	if (saved[S_CHECKPOINTS].get<std::chrono::steady_clock::duration>(duration) > duration)
		saved[S_CHECKPOINTS] = duration;
	saved[S_CHECKPOINTS].truncate();
	started = false;
	AHydroCarGameModeBase::instance->respawn(this);
	OnEndGame();
	saved[S_STATISTICS]["races"].get<int>()++;
	saved[S_STATISTICS]["time"]["raced"].get<std::chrono::steady_clock::duration>() += duration;
}

void AHydroCarPawn::dropCheckpoint()
{
	if (saved[S_CHECKPOINTS].size() > 1)
		saved[S_CHECKPOINTS].getList().pop_back();
	saved[S_STATISTICS]["checkpoint"]["dropped"].get<std::chrono::steady_clock::duration>()++;
}

void AHydroCarPawn::saveCheckpoint()
{
	SaveData sd(std::chrono::steady_clock::now() - startTime);
	sd["nextCheckPoint"] = nextCheckPoint;
	sd["currentLap"] = currentLap;
	sd["pos"] = GetActorLocation();
	sd["rot"] = GetActorRotation();
	GetVehicleMovementComponent()->GetBaseSnapshot(sd["mov"]);
	OnSaveCheckpoint({&sd});
	saved[S_CHECKPOINTS].push(sd);
	++saved[S_STATISTICS]["checkpoint"]["saved"].get<int>();
}

void AHydroCarPawn::loadCheckpoint()
{
	if (!saved[S_CHECKPOINTS].size())
		return;
	auto &sd = saved[S_CHECKPOINTS].getList().back();
	auto newStartTime = std::chrono::steady_clock::now() - sd.get<std::chrono::steady_clock::duration>();
	saved[S_STATISTICS]["time"]["dropped"].get<std::chrono::steady_clock::duration>() += newStartTime - startTime;
	startTime = newStartTime;
	nextCheckPoint = sd["nextCheckPoint"];
	currentLap = sd["currentLap"];
	TeleportTo(sd["pos"], sd["rot"], false, false);
	GetVehicleMovementComponent()->SetBaseSnapshot(sd["mov"]);
	OnLoadCheckpoint({&sd});
	saved[S_STATISTICS]["checkpoint"]["loaded"].get<int>()++;
}
