// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameModeUtility.generated.h"

DECLARE_DYNAMIC_DELEGATE(FTransition)
DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FUpdate)
DECLARE_DYNAMIC_DELEGATE_OneParam(FSmoothTransition, float, deltaSeconds)
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FSmoothUpdate, float, deltaSeconds)

/**
 *
 */
UCLASS()
class HYDROCAR_API AGameModeUtility : public AGameMode
{
	GENERATED_BODY()

public:
	AGameModeUtility() {
        PrimaryActorTick.bCanEverTick = true;
    }

    // Call the callback every tick for duration seconds
    UFUNCTION(Category=Execution, BlueprintCallable)
    void AddSmoothTransition(FSmoothTransition callback, float duration) {
		smoothTransitions = new SmoothTransition{smoothTransitions, callback, duration};
	}
    // Call the callback 2 times per second for duration/2 seconds
    UFUNCTION(Category=Execution, BlueprintCallable)
	void AddCheapTransition(FTransition callback, int duration) {
		transitions = new Transition{transitions, callback, duration};
	}
	// Call the callback every tick until it return true
	UFUNCTION(Category=Execution, BlueprintCallable)
	void AddSmoothUpdate(FSmoothUpdate callback) {
		smoothUpdates = new SmoothUpdate{smoothUpdates, callback};
	}
	// Call the callback 2 times per second until it return true
	UFUNCTION(Category=Execution, BlueprintCallable)
	void AddCheapUpdate(FUpdate callback) {
		updates = new Update{updates, callback};
	}

	UFUNCTION(Category=Execution, BlueprintCallable)
	void CancelSmoothTransition(FSmoothTransition callback) {
		for (auto it = &smoothTransitions; auto tmp = *it; it = &tmp->next) {
			if (tmp->callback == callback) {
				*it = tmp->next;
				delete tmp;
				return;
			}
		}
	}
	UFUNCTION(Category=Execution, BlueprintCallable)
	void CancelCheapTransition(FTransition callback) {
		for (auto it = &transitions; auto tmp = *it; it = &tmp->next) {
			if (tmp->callback == callback) {
				*it = tmp->next;
				delete tmp;
				return;
			}
		}
	}
	UFUNCTION(Category=Execution, BlueprintCallable)
	void CancelSmoothUpdate(FSmoothUpdate callback) {
		for (auto it = &smoothUpdates; auto tmp = *it; it = &tmp->next) {
			if (tmp->callback == callback) {
				*it = tmp->next;
				delete tmp;
				return;
			}
		}
	}
	UFUNCTION(Category=Execution, BlueprintCallable)
	void CancelCheapUpdate(FUpdate callback) {
		for (auto it = &updates; auto tmp = *it; it = &tmp->next) {
			if (tmp->callback == callback) {
				*it = tmp->next;
				delete tmp;
				return;
			}
		}
	}

    virtual void Tick(float deltaSeconds) override;
private:
	float timer = 0;
	struct SmoothTransition {
		SmoothTransition *next;
		FSmoothTransition callback;
		float duration;
	} *smoothTransitions = nullptr;

	struct Transition {
		Transition *next;
		FTransition callback;
		int duration;
	} *transitions = nullptr;

	struct SmoothUpdate {
		SmoothUpdate *next;
		FSmoothUpdate callback;
	} *smoothUpdates = nullptr;

	struct Update {
		Update *next;
		FUpdate callback;
	} *updates = nullptr;
};
