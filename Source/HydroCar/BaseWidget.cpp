// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWidget.h"

void UBaseWidget::Bind(AHydroCarPawn *_player)
{
    player = _player;
    if (_player->display)
        _player->display->Close();
    _player->display = this;
    nextControlDependency = 0;
    updateControlDependency(0);
    player->updateControl(controlDependency);
}

void UBaseWidget::Open(UBaseWidget *widget)
{
    widget->parent = this;
    widget->player = player;
    const int sz = childs.Num();
    UE_LOG(LogTemp, Warning, TEXT("Open Widget %s in %s, totalizing %i"), *widget->GetName(), *GetName(), sz);
    for (int i = 0; i < sz; ++i) {
        if (widget->priorityLevel < childs[i]->priorityLevel) {
            expandControl(this, childs.Insert(widget, i) + 1);
        }
    }
    expandControl(this, childs.Add(widget) + 1);
}

void UBaseWidget::OnBack_Implementation()
{
    Close();
}

void UBaseWidget::detach()
{
    updateControlDependency(WC_DETACHED);
    for (auto &c : childs)
        c->detach();
    childs.Empty();
}

void UBaseWidget::Close()
{
    if (!this) {
        UE_LOG(LogTemp, Error, TEXT("Attempt to close a null widget"));
        return;
    }
    detach();
    if (parent) {
        int idx = parent->childs.Find(this);
        UE_LOG(LogTemp, Warning, TEXT("REMOVING %s at index %i"), *GetName(), idx);
        parent->childs.RemoveAt(idx);
        for (auto p : parent->childs) {
            UE_LOG(LogTemp, Display, TEXT("Remaining %s"), *p->GetName());
        }
        expandControl(parent, idx);
    } else {
        // UE_LOG(LogTemp, Warning, TEXT("Closing main window"));
        player->display = nullptr;
        player->updateControl(0);
    }
}

void UBaseWidget::expandControl(UBaseWidget *self, int index)
{
    int8 tmp = (index == self->childs.Num()) ? self->nextControlDependency : self->childs[index]->controlDependency | self->childs[index]->control;
    BEGIN:
    if (self->setControlDependency(index, tmp)) {
        self->player->applyControl();
    } else {
        tmp = self->controlDependency | self->control;
        if (self->parent) {
            index = self->parent->childs.Find(self);
            self = self->parent;
            goto BEGIN;
        } else
            self->player->updateControl(tmp);
    }
}

bool UBaseWidget::setControlDependency(int index, int8 _nextControlDependency)
{
    while (index--) {
        auto &c = *childs[index];
        const int tmp = c.childs.Num();
        if (tmp) {
            c.nextControlDependency = _nextControlDependency;
            if (c.setControlDependency(tmp, _nextControlDependency))
                return true;
            _nextControlDependency = c.controlDependency | c.control;
        } else {
            c.nextControlDependency = _nextControlDependency;
            if (c.updateControlDependency(_nextControlDependency))
                return true;
            _nextControlDependency |= c.control;
        }
    }
    return updateControlDependency(_nextControlDependency);
}

FReply UBaseWidget::NativeOnKeyUp(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P) {
        OnBack();
        Super::NativeOnKeyUp(InGeometry, InKeyEvent);
        return FReply::Handled();
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}
