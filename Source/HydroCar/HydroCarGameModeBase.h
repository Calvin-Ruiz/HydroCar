// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Tools/BigSave.hpp"
#include "HydroCarGameModeBase.generated.h"

class AHydroCarPawn;

USTRUCT()
struct FAchievementDescription {
	GENERATED_BODY()
public:
	// name of this achievement
	UPROPERTY(EditAnywhere)
	FString name;
	// description of this achievement
	UPROPERTY(EditAnywhere)
	FString description;
	// minimal progress value to unlock this achievement
	UPROPERTY(EditAnywhere)
	int completion = 1;
	// minimal progress to reach before displaying this achievement
	UPROPERTY(EditAnywhere)
	int minForVisiblity = 0;
	// number of useless coin gained on completion
	UPROPERTY(EditAnywhere)
	int ucCompletion = 100;
	// number of useless coin gained on re-completion
	UPROPERTY(EditAnywhere)
	int ucRecompletion = 0;
};

UENUM()
enum AchievementName {
    RACE_COMPLETED,
    RACE_UNCOMPLETED,
    RETRY,
    RECTIFY,
    PERFECTIONNIST,
    UNCERTAIN,
    BACKWARD,
    RETRY_FOREVER,
    ACHIEVEMENT_COLLECTOR,
    // Insert new achievements right before this comment
    COUNT,
};

#define ALL_ACHIEVEMENTS { \
    FAchievementDescription{"Race Completed", "Complete a race", 1, 0, 100, 100}, \
    FAchievementDescription{"Race Uncompleted", "Complete a race... In reverse order", 1, 1, 500, 150}, \
    FAchievementDescription{"Retry", "Go back to a checkpoint", 1, 1, 100, 0}, \
    FAchievementDescription{"Rectify", "Drop a checkpoint", 1, 1, 100, 0}, \
    FAchievementDescription{"Perfectionnist", "Drop several checkpoints", 10, 1, 300, 10}, \
    FAchievementDescription{"Uncertain", "Reach previous checkpoint multiple times", 5, 2, 300, 30}, \
    FAchievementDescription{"Backward", "Go backward", 1, 1, 100, 0}, \
    FAchievementDescription{"Retry Forever", "Retry again, again, again...", 100, 10, 1000, 10}, \
    FAchievementDescription{"Achievement Collector", "Complete all the achievements once, excepted this one", int(AchievementName::COUNT) - 1, 5, 3000, 0} \
}

/**
 *
 */
UCLASS()
class HYDROCAR_API AHydroCarGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	static AHydroCarGameModeBase *instance;

	virtual void StartPlay() override;

	void respawn(AHydroCarPawn *self);

	UPROPERTY(EditDefaultsOnly, EditFixedSize)
	TArray<FAchievementDescription> achievements = ALL_ACHIEVEMENTS;

private:
	std::shared_ptr<BigSave> save;
};
