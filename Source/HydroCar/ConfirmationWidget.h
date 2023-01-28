// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseWidget.h"
#include "ConfirmationWidget.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnConfirm);

UCLASS()
class HYDROCAR_API UConfirmationWidget : public UBaseWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static void RequestConfirmation(UBaseWidget *target, const FString &name, FText title, FText description, FOnConfirm onConfirm);

    UFUNCTION(BlueprintImplementableEvent)
    void SetContent(const FText &title, const FText &description);

    UFUNCTION(BlueprintCallable)
    void Confirm();

    UPROPERTY(BlueprintReadWrite)
    bool remember = false;

// PROTECTED
    UPROPERTY(BlueprintReadOnly)
    FSaveData brain;
private:
    FOnConfirm onConfirm;
};
