// Fill out your copyright notice in the Description page of Project Settings.


#include "HydroCarPawn.h"
#include "Interactive.h"
#include "HydroCarGameModeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Tools/BigSave.hpp"
#include "BaseWidget.h"
#include "ConfirmationWidget.h"

AHydroCarPawn::AHydroCarPawn()
{
	UChaosWheeledVehicleMovementComponent* Vehicle4W = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	// Adjust the tire
	//Vehicle4W->MinNormalizedTireLoad = MinTireLoad;
	//Vehicle4W->MinNormalizedTireLoadFiltered = MinTireLoadFiltered;
	//Vehicle4W->MaxNormalizedTireLoad = MaxTireLoad;
	//Vehicle4W->MaxNormalizedTireLoadFiltered = MaxTireLoadFiltered;

	// Torque setup
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

void AHydroCarPawn::loadConfig(const FString &playerName)
{
	UE_LOG(LogTemp, Warning, TEXT("Loading config for player %s, note that %p"), *playerName, Saved.handle);
	if (!saved.getSaveName().empty())
		saved.store();
	saved.reset();
	saved.open(TCHAR_TO_UTF8(*playerName));
	UChaosWheeledVehicleMovementComponent* Vehicle4W = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
	Vehicle4W->EngineSetup.MaxRPM = saved[S_STATISTICS]["MaxRPM"].get<float>(MaxEngineRPM);
	Hydrogen = HydrogenCapacity = saved[S_STATISTICS]["H2"]["capacity"].get<float>(10);
	HydrogenRenegenation = saved[S_STATISTICS]["H2"]["recovery"].get<float>(0);
	HydrogenThruster = saved[S_STATISTICS]["H2"]["thrust"].get<float>(2);
	viewRotationSpeed = saved[S_SETTINGS]["viewSpeed"].get<float>(viewRotationSpeed);
	ConfirmationSkip.handle = &saved[S_SETTINGS]["confirm"];
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
		FBaseSnapshotData data;
		GetVehicleMovementComponent()->GetBaseSnapshot(data);
		data.AngularVelocity *= std::pow(0.5/HydrogenThruster, deltaTime);
		const float acceleration = deltaTime * HydrogenThruster * 50;
		auto forward = GetMesh()->GetForwardVector();
		if (leftThrust) {
			if (rightThrust) {
				data.LinearVelocity += GetMesh()->GetUpVector() * (acceleration * 2);
			} else {
				data.AngularVelocity += forward * (-0.03 * acceleration);
				data.LinearVelocity += GetMesh()->GetUpVector() * acceleration;
			}
		} else if (rightThrust) {
			data.AngularVelocity += forward * (0.03 * acceleration);
			data.LinearVelocity += GetMesh()->GetUpVector() * acceleration;
		}
		if (backThrust) {
			data.LinearVelocity += forward * acceleration;
		}
		GetVehicleMovementComponent()->SetBaseSnapshot(data);
		Hydrogen += (HydrogenRenegenation - HydrogenThruster) * deltaTime;
		if (Hydrogen <= 0) {
			Hydrogen = 0;
			UsingHydrogen = false;
			updateAchievement(AchievementName::EMPTY);
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

	bool isQwerty = saved[S_SETTINGS]["qwerty"];
	PlayerInputComponent->BindAxis("Throttle", this, &AHydroCarPawn::ApplyThrottle);
	PlayerInputComponent->BindAxis(isQwerty ? "Steer2" : "Steer", this, &AHydroCarPawn::ApplySteering);
	PlayerInputComponent->BindAxis("LookUp", this, &AHydroCarPawn::Lookup);
	PlayerInputComponent->BindAxis("Turn", this, &AHydroCarPawn::Turn);

	PlayerInputComponent->BindAction("Handbrake", IE_Pressed, this, &AHydroCarPawn::OnHandbrakePressed);
	PlayerInputComponent->BindAction("Handbrake", IE_Released, this, &AHydroCarPawn::OnHandbrakeReleased);
	PlayerInputComponent->BindAction("Respawn", IE_Released, this, &AHydroCarPawn::loadCheckpoint);
	PlayerInputComponent->BindAction("DropCheckpoint", IE_Pressed, this, &AHydroCarPawn::dropCheckpoint);
	PlayerInputComponent->BindAction("DropCheckpoint", IE_Released, this, &AHydroCarPawn::loadCheckpoint);
	PlayerInputComponent->BindAction("Back", IE_Released, this, &AHydroCarPawn::OnBack);
	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &AHydroCarPawn::OnBoostTrue);
	PlayerInputComponent->BindAction("Boost", IE_Released, this, &AHydroCarPawn::OnBoostFalse);
	PlayerInputComponent->BindAction(isQwerty ? "LeftThrust2" : "LeftThrust", IE_Pressed, this, &AHydroCarPawn::OnLeftThrustTrue);
	PlayerInputComponent->BindAction(isQwerty ? "LeftThrust2" : "LeftThrust", IE_Released, this, &AHydroCarPawn::OnLeftThrustFalse);
	PlayerInputComponent->BindAction("RightThrust", IE_Pressed, this, &AHydroCarPawn::OnRightThrustTrue);
	PlayerInputComponent->BindAction("RightThrust", IE_Released, this, &AHydroCarPawn::OnRightThrustFalse);
}

void AHydroCarPawn::BecomeViewTarget(APlayerController *PC)
{
	loadConfig(TEXT("Player") + FString::FromInt(PC->GetLocalPlayer()->GetLocalPlayerIndex()));
	Super::BecomeViewTarget(PC);
	auto tmp = std::to_string(PC->GetLocalPlayer()->GetLocalPlayerIndex());
	static_cast<UBaseWidget *>(UUserWidget::CreateWidgetInstance(*PC, overlay, FName(("Overlay"+tmp).data())))->Bind(this);
	if (pauseMenuClass)
		pauseMenu = static_cast<UBaseWidget *>(UUserWidget::CreateWidgetInstance(*PC, pauseMenuClass, FName(("PauseMenu"+tmp).data())));
	if (mainMenuClass) {
		mainMenu = static_cast<UBaseWidget *>(UUserWidget::CreateWidgetInstance(*PC, mainMenuClass, FName(("MainMenu"+tmp).data())));
		display->Open(mainMenu);
	}
	if (confirmationClass)
		confirmation = static_cast<UConfirmationWidget *>(UUserWidget::CreateWidgetInstance(*PC, confirmationClass, FName(("Confirmation"+tmp).data())));
}

void AHydroCarPawn::EndViewTarget(APlayerController *PC)
{
	UE_LOG(LogTemp, Warning, TEXT("End of Player%i"));
	display->Close();
	pauseMenu = nullptr;
	mainMenu = nullptr;
	confirmation = nullptr;
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
		AddControllerPitchInput(val * viewRotationSpeed);
	}
}

void AHydroCarPawn::Turn(float val)
{
	if (val != 0.f) {
		AddControllerYawInput(val * viewRotationSpeed);
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
	currentLap = 1;
	nextCheckPoint = 0;
	prevCheckPoint = checkPointCount - 1;
	lastRPM = -10;
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
	UChaosWheeledVehicleMovementComponent* Vehicle4W = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
	Vehicle4W->EngineSetup.MaxRPM = saved[S_STATISTICS]["MaxRPM"].get<float>(MaxEngineRPM);
	Hydrogen = HydrogenCapacity = saved[S_STATISTICS]["H2"]["capacity"].get<float>(HydrogenCapacity);
	HydrogenRenegenation = saved[S_STATISTICS]["H2"]["recovery"].get<float>(HydrogenRenegenation);
	HydrogenThruster = saved[S_STATISTICS]["H2"]["thrust"].get<float>(HydrogenThruster);
	viewRotationSpeed = saved[S_SETTINGS]["viewSpeed"].get<float>(viewRotationSpeed);
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
	timer = "Timer: 00:00.000";
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
	timer = "Timer: 00:00.000";
	started = false;
	onBegin();
}

void AHydroCarPawn::pauseToMainMenu()
{
	if (saved[S_CHECKPOINTS].size() > 0)
		saveCheckpointInternal();
	pauseMenu->Close();
	auto tmp = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetSnapshot();
	tmp.AngularVelocity = tmp.LinearVelocity = FVector::Zero();
	tmp.EngineRPM = 0;
	static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->SetSnapshot(tmp);
	timer = "Timer: 00:00.000";
	started = false;
	OnEndGame();
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
	auto tmp = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetSnapshot();
	sd["mov"] = static_cast<FBaseSnapshotData&>(tmp);
	sd["rpm"] = tmp.EngineRPM;
	sd["gear"] = tmp.SelectedGear;
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
	auto tmp = static_cast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovementComponent())->GetSnapshot();
	static_cast<FBaseSnapshotData&>(tmp) = sd["mov"];
	tmp.EngineRPM = sd["rpm"];
	tmp.SelectedGear = sd["gear"];
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
	while (saved[S_ACHIEVEMENTS].size() < AchievementName::COUNT)
		saved[S_ACHIEVEMENTS].push();
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
			startTime = std::chrono::steady_clock::now() - pauseRememberance;
		}
	} else if (WC_PAUSING & gained) {
		pauseRememberance = std::chrono::steady_clock::now() - startTime;
		GetLocalViewingPlayerController()->SetPause(true);
	}
	controlDependency = newControl;
	applyControl();
}
