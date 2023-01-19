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
#include "BaseWidget.h"

AHydroCarPawn::AHydroCarPawn()
{
	UChaosWheeledVehicleMovementComponent* Vehicle4W = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	// Adjust the tire
	//Vehicle4W->MinNormalizedTireLoad = MinTireLoad;
	//Vehicle4W->MinNormalizedTireLoadFiltered = MinTireLoadFiltered;
	//Vehicle4W->MaxNormalizedTireLoad = MaxTireLoad;
	//Vehicle4W->MaxNormalizedTireLoadFiltered = MaxTireLoadFiltered;

	// Torque setup
	Vehicle4W->EngineSetup.MaxRPM = saved[S_STATISTICS]["MaxRPM"].get<float>(MaxEngineRPM);
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

	if (UsingHydrogen) {
		Hydrogen -= deltaTime;
		if (Hydrogen <= 0) {
			Hydrogen = 0;
			UsingHydrogen = false;
		}
	} else if (Hydrogen < HydrogenCapacity) {
		Hydrogen += HydrogenRenegenation * deltaTime;
		if (Hydrogen > HydrogenCapacity)
			Hydrogen = HydrogenCapacity;
	}

	float RPM = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetEngineRotationSpeed();
	if (abs(RPM - lastRPM) >= 10) {
		UpdateRPM(RPM);
		lastRPM = RPM;
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
	PlayerInputComponent->BindAction("Back", IE_Released, this, &AHydroCarPawn::OnBack);
	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &AHydroCarPawn::OnBoost);

}

void AHydroCarPawn::BecomeViewTarget(APlayerController *PC)
{
	Super::BecomeViewTarget(PC);
	static_cast<UBaseWidget *>(UUserWidget::CreateWidgetInstance(*PC, overlay, FName("Overlay")))->Bind(this);
	if (pauseMenuClass)
		pauseMenu = static_cast<UBaseWidget *>(UUserWidget::CreateWidgetInstance(*PC, pauseMenuClass, FName("PauseMenu")));
	if (mainMenuClass) {
		mainMenu = static_cast<UBaseWidget *>(UUserWidget::CreateWidgetInstance(*PC, mainMenuClass, FName("MainMenu")));
		display->Open(mainMenu);
	}
}

void AHydroCarPawn::EndViewTarget(APlayerController *PC)
{
	display->Close();
	pauseMenu = nullptr;
	mainMenu = nullptr;
	Super::EndViewTarget(PC);
}

void AHydroCarPawn::setDirection(int32 direction)
{
	if (direction != cachedDirection) {
		cachedDirection = direction;
		GetVehicleMovementComponent()->SetTargetGear(direction, true);
		if (direction < 0)
			updateAchievement(AchievementName::BACKWARD);
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

void AHydroCarPawn::OnBack()
{
	if (pauseMenu)
		display->Open(pauseMenu);
}

void AHydroCarPawn::OnBoost()
{

}

bool AHydroCarPawn::reachCheckpoint(int checkPointNumber)
{
	if (checkPointNumber == nextCheckPoint) {
		if (!started) {
			startTime = std::chrono::steady_clock::now();
			started = true;
		} else if (currentLap < 0)
			updateAchievement(AchievementName::UNCERTAIN);
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
		} else if (currentLap > 0)
			updateAchievement(AchievementName::UNCERTAIN);
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
	UE_LOG(LogTemp, Display, TEXT("Begin Play"));
	currentLap = 1;
	nextCheckPoint = 0;
	prevCheckPoint = checkPointCount - 1;
	lastRPM = -10;
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
	Hydrogen = HydrogenCapacity = saved[S_STATISTICS]["H2"]["capacity"].get<float>(HydrogenCapacity);
	HydrogenRenegenation = saved[S_STATISTICS]["H2"]["recovery"].get<float>(HydrogenRenegenation);
	if (saved[S_CHECKPOINTS].size()) {
		UE_LOG(LogTemp, Display, TEXT("Load Saved Checkpoint"));
		loadCheckpointInternal();
		started = true;
	}
	OnBeginGame();
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
	auto tmp = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetSnapshot();
	tmp.AngularVelocity = tmp.LinearVelocity = FVector::Zero();
	tmp.EngineRPM = 0;
	static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->SetSnapshot(tmp);
	OnEndGame();
	saved[S_STATISTICS]["races"].get<int>()++;
	saved[S_STATISTICS]["time"]["raced"].get<std::chrono::steady_clock::duration>() += duration;
	if (currentLap > 0) {
		updateAchievement(AchievementName::RACE_COMPLETED);
	} else {
		updateAchievement(AchievementName::RACE_UNCOMPLETED);
	}
}

void AHydroCarPawn::onRestart()
{
	saved[S_CHECKPOINTS].truncate();
	AHydroCarGameModeBase::instance->respawn(this);
	auto tmp = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetSnapshot();
	tmp.AngularVelocity = tmp.LinearVelocity = FVector::Zero();
	tmp.EngineRPM = 0;
	static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->SetSnapshot(tmp);
	started = false;
	onBegin();
}

void AHydroCarPawn::dropCheckpoint()
{
	if (saved[S_CHECKPOINTS].size() > 1) {
		saved[S_CHECKPOINTS].getList().pop_back();
		saved[S_STATISTICS]["checkpoint"]["dropped"].get<int>()++;
		updateAchievement(AchievementName::RECTIFY);
		updateAchievement(AchievementName::PERFECTIONNIST);
	}
}

void AHydroCarPawn::saveCheckpointInternal()
{
	SaveData sd(std::chrono::steady_clock::now() - startTime);
	sd["prevCheckPoint"] = prevCheckPoint;
	sd["nextCheckPoint"] = nextCheckPoint;
	sd["currentLap"] = currentLap;
	sd["hydrogen"] = Hydrogen;
	sd["pos"] = GetActorLocation();
	sd["rot"] = GetActorRotation();
	GetVehicleMovementComponent()->GetBaseSnapshot(sd["mov"]);
	sd["rpm"] = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetEngineRotationSpeed();
	OnSaveCheckpoint({&sd});
	saved[S_CHECKPOINTS].push(sd);
}

void AHydroCarPawn::saveCheckpoint()
{
	saveCheckpointInternal();
	++saved[S_STATISTICS]["checkpoint"]["saved"].get<int>();
}

void AHydroCarPawn::loadCheckpointInternal()
{
	auto &sd = saved[S_CHECKPOINTS].getList().back();
	if (started) {
		auto newStartTime = std::chrono::steady_clock::now() - sd.get<std::chrono::steady_clock::duration>();
		saved[S_STATISTICS]["time"]["dropped"].get<std::chrono::steady_clock::duration>() += newStartTime - startTime;
		startTime = newStartTime;
	} else
		startTime = std::chrono::steady_clock::now();
	prevCheckPoint = sd["prevCheckPoint"];
	nextCheckPoint = sd["nextCheckPoint"];
	currentLap = sd["currentLap"];
	Hydrogen = sd["hydrogen"];
	TeleportTo(sd["pos"], sd["rot"], false, false);
	GetVehicleMovementComponent()->SetBaseSnapshot(sd["mov"]);
	auto tmp = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetSnapshot();
	tmp.EngineRPM = sd["rpm"];
	static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->SetSnapshot(tmp);
	OnLoadCheckpoint({&sd});
}

void AHydroCarPawn::loadCheckpoint()
{
	if (!saved[S_CHECKPOINTS].size())
		return;
	loadCheckpointInternal();
	saved[S_STATISTICS]["checkpoint"]["loaded"].get<int>()++;
	updateAchievement(AchievementName::RETRY);
	updateAchievement(AchievementName::RETRY_FOREVER);
}

void AHydroCarPawn::updateAchievement(AchievementName name)
{
	if (saved[S_ACHIEVEMENTS].empty()) {
		for (int i = 0; i < AchievementName::COUNT; ++i)
			saved[S_ACHIEVEMENTS].push();
	}
	auto &desc = AHydroCarGameModeBase::instance->achievements[name];
	int &progress = saved[S_ACHIEVEMENTS][name];
	// UE_LOG(LogTemp, Warning, TEXT("Update Achievement %s"), *desc.name);
	if (++progress % desc.completion == 0) {
		if (progress == desc.completion) {
			OnAchievement({desc.name, desc.description, desc.completion, desc.completion, desc.ucCompletion}, true);
			saved[S_STATISTICS]["uc"].get<int>() += desc.ucCompletion;
			updateAchievement(AchievementName::ACHIEVEMENT_COLLECTOR);
		} else if (desc.ucRecompletion) {
			UE_LOG(LogTemp, Display, TEXT("Recompletion grant %i"), desc.ucRecompletion);
			OnAchievement({desc.name, desc.description, progress, desc.completion, desc.ucRecompletion}, false);
			saved[S_STATISTICS]["uc"].get<int>() += desc.ucRecompletion;
		}
	}
}

TArray<FAchievementDisplay> AHydroCarPawn::queryAchievements()
{
	TArray<FAchievementDisplay> ret;
	auto &sd = saved[S_ACHIEVEMENTS];
	auto &ac = AHydroCarGameModeBase::instance->achievements;
	const int sz = sd.size();
	for (int i = 0; i < sz; ++i) {
		auto &desc = ac[i];
		int progress = sd[i];
		if (progress >= desc.minForVisiblity)
			ret.Add({desc.name, desc.description, progress, desc.completion, (progress < desc.completion) ? desc.ucCompletion : desc.ucRecompletion});
	}
	return ret;
}

void AHydroCarPawn::applyControl()
{
	if (controlDependency & WC_INPUT) {
		// UE_LOG(LogTemp, Warning, TEXT("Transfer focus to %p"), newInputTarget);
		newInputTarget->bIsFocusable = true;
		if (controlDependency & WC_OVERRIDE_DISPLAY) {
			newInputTarget->GetOwningPlayer()->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(newInputTarget->TakeWidget()));
		} else
			newInputTarget->GetOwningPlayer()->SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(newInputTarget->TakeWidget()));
	}
}

void AHydroCarPawn::updateControl(int8 newControl)
{
	// UE_LOG(LogTemp, Warning, TEXT("UPDATE CONTROL %i -> %i"), controlDependency, newControl);
	int8 gained = ~controlDependency & newControl;
	int8 lost = controlDependency & ~newControl;

	if (WC_INPUT & lost) {
		if (auto pc = GetLocalViewingPlayerController()) {
			// UE_LOG(LogTemp, Warning, TEXT("Player get input"));
			pc->bShowMouseCursor = false;
			pc->SetInputMode(FInputModeGameOnly());
		}
	} else if (WC_INPUT & gained) {
		// UE_LOG(LogTemp, Warning, TEXT("Player loose input"));
		GetLocalViewingPlayerController()->bShowMouseCursor = true;
	}
	if (WC_PAUSING & lost) {
		if (auto pc = GetLocalViewingPlayerController()) {
			pc->SetPause(false);
			loadCheckpointInternal();
		}
		saved[S_CHECKPOINTS].getList().pop_back();
	} else if (WC_PAUSING & gained) {
		saveCheckpointInternal();
		GetLocalViewingPlayerController()->SetPause(true);
	}
	controlDependency = newControl;
	applyControl();
}
