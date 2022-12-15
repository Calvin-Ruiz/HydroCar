// Fill out your copyright notice in the Description page of Project Settings.


#include "HydroCarPawn.h"
#include "Interactive.h"
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
