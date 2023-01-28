// Fill out your copyright notice in the Description page of Project Settings.

#include "GameModeUtility.h"

void AGameModeUtility::Tick(float deltaSeconds)
{
    if ((timer += deltaSeconds) >= 0.5) {
        timer -= 0.5;
        for (auto it = &transitions; auto tmp = *it;) {
            tmp->callback.Execute();
            if (--tmp->duration) {
                it = &tmp->next;
            } else {
                *it = tmp->next;
                delete tmp;
            }
        }
        for (auto it = &updates; auto tmp = *it;) {
            if (tmp->callback.Execute()) {
                *it = tmp->next;
                delete tmp;
            } else {
                it = &tmp->next;
            }
        }
    }
    for (auto it = &smoothTransitions; auto tmp = *it;) {
        if (tmp->duration <= deltaSeconds) {
            tmp->callback.Execute(tmp->duration);
            *it = tmp->next;
            delete tmp;
        } else {
            tmp->callback.Execute(deltaSeconds);
            tmp->duration -= deltaSeconds;
            it = &tmp->next;
        }
    }
    for (auto it = &smoothUpdates; auto tmp = *it;) {
        if (tmp->callback.Execute(deltaSeconds)) {
            *it = tmp->next;
            delete tmp;
        } else {
            it = &tmp->next;
        }
    }
}
