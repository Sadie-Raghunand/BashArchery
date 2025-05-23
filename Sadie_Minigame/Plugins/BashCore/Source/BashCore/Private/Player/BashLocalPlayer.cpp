// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BashLocalPlayer.h"

void UBashLocalPlayer::PlayerAdded(class UGameViewportClient* InViewportClient, FPlatformUserId InUserId)
{
	Super::PlayerAdded(InViewportClient, InUserId);

	PlayerData = NewObject<UPlayerData>(this);
}

void UBashLocalPlayer::PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID)
{
	Super::PlayerAdded(InViewportClient, InControllerID);

	PlayerData = NewObject<UPlayerData>(this);
}
