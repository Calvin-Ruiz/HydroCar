// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HydroCarPawn.h"
#include "Templates/SharedPointer.h"
#include "UETools/FSaveData.h"
#include "BaseWidget.generated.h"

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum WidgetControl {
	// Temporarily close lower-priority widget while open
	WC_OVERRIDE_STATE = 0x01,
	// Prevent lower-priority widget to display
	WC_OVERRIDE_DISPLAY = 0x02,
	// Use keyboard and mouse input
	WC_INPUT = 0x04,
	// Pause the game while open
	WC_PAUSING = 0x08,
};

// Widget Control dependency value to set in order to detach a widget
#define WC_DETACHED (WC_OVERRIDE_STATE | WC_OVERRIDE_DISPLAY | WC_INPUT)

/**
 *
 */
UCLASS()
class HYDROCAR_API UBaseWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	bool operator<(const UBaseWidget &other) {
		return priorityLevel < other.priorityLevel;
	}
	// Bind a new widget to the player
	UFUNCTION(BlueprintCallable)
	void Bind(class AHydroCarPawn *_player);
	// Open a widget here
	UFUNCTION(BlueprintCallable)
	void Open(UBaseWidget *widget);
	// Close this widget
	UFUNCTION(BlueprintCallable)
	void Close();
	void detach();

	// Called when shown
	UFUNCTION(BlueprintImplementableEvent)
	void OnShow();

	// Called when no longer shown
	UFUNCTION(BlueprintImplementableEvent)
	void OnHide();

	// Called when the back key is pressed
	UFUNCTION(BlueprintNativeEvent)
	void OnBack();
	void OnBack_Implementation();

	// Utility function for transmitting information between BaseWidget
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnInform(UBaseWidget *target, FSaveData args);

	virtual FReply NativeOnKeyUp(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) override;
protected:
	// Owning player
	UPROPERTY(BlueprintReadOnly)
	class AHydroCarPawn *player;

	// Priority for display
	UPROPERTY(Category = Menu, EditDefaultsOnly)
	int priorityLevel = 0;

	// Determine how this widget interact with the player and other widgets
	UPROPERTY(Category = Menu, EditDefaultsOnly, meta = (Bitmask, BitmaskEnum = WidgetControl))
	int8 control = 0;
private:
	static void expandControl(UBaseWidget *self, int index);
	// Set the control dependency, return true if the controlDependency of this widget kept unchanged
	bool setControlDependency(int index, int8 _nextControlDependency);
	UBaseWidget *parent = nullptr;
	UPROPERTY()
	TArray<UBaseWidget *> childs;

	// Cumulated control of higher priority widgets
	int8 controlDependency = WC_DETACHED;
	// Cumulated control of higher priority widgets of the widget after this one
	int8 nextControlDependency = 0;

	// Update the control dependency, return true if the control dependency is unchanged
	inline bool updateControlDependency(int8 newDependency) {
		if (newDependency == controlDependency)
			return true;
		UE_LOG(LogTemp, Warning, TEXT("Update Dependency %i -> %i"), controlDependency, newDependency);
		int8 gained = ~controlDependency & newDependency;
		int8 lost = controlDependency & ~newDependency;
		if (WC_OVERRIDE_DISPLAY & gained) {
			if (WC_OVERRIDE_STATE & gained)
				OnHide();
			RemoveFromParent();
		} else if (WC_OVERRIDE_DISPLAY & lost) {
			UE_LOG(LogTemp, Warning, TEXT("Add to player screen"));
			AddToPlayerScreen(priorityLevel);
			if (WC_OVERRIDE_STATE & lost)
				OnShow();
		}
		if (WC_INPUT & control & lost)
			player->setNewInputTarget(this);
		controlDependency = newDependency;
		return false;
	}
};
