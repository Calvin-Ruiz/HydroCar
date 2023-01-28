// Fill out your copyright notice in the Description page of Project Settings.

#include "ConfirmationWidget.h"

void UConfirmationWidget::RequestConfirmation(UBaseWidget *target, const FString &name, FText title, FText description, FOnConfirm onConfirm)
{
    SaveData &sd = (*target->player->ConfirmationSkip.handle)[TCHAR_TO_UTF8(*name)];
    if (sd.get<bool>()) {
        onConfirm.Execute();
    } else {
        auto *self = target->player->confirmation;
        self->remember = false;
        self->brain.handle = &sd;
        self->onConfirm = onConfirm;
        self->SetContent(title, description);
        target->Open(self);
    }
}

void UConfirmationWidget::Confirm()
{
    if (remember)
        brain.handle->get<bool>() = true;
    Close();
    onConfirm.Execute();
}
