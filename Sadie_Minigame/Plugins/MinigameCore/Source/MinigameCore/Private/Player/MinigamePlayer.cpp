/**************************************************************************************************
 * MinigamePlayer.cpp
 * LIMBITLESS SOLUTIONS INC.                                                              2/21/2025
 * https://limbitless-solutions.org/                                                         
 * 
 * Author:  Donny Akers
 * 
 * Copyright (C) 2025 Limbitless Solutions Inc.
 * Copyright (C) 2025 University of Central Florida
 * 
 * This work is licensed under the:
 * Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/ 
 * or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 */

#include "Player/MinigamePlayer.h"
#include <Kismet/GameplayStatics.h>
#include "EnhancedInputComponent.h"
#include "MinigameBase.h"
#include <EnhancedInputSubsystems.h>

AMinigamePlayer::AMinigamePlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}

void AMinigamePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (auto PlayerController = Cast<APlayerController>(Controller))
	{
		using InputSubsystem = UEnhancedInputLocalPlayerSubsystem;
		if (InputSubsystem* Subsystem = ULocalPlayer::GetSubsystem<InputSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (Input && ReadyAction)
	{
		Input->BindAction(ReadyAction, ETriggerEvent::Triggered, this, &AMinigamePlayer::ReadyPlayer);
	}
}

void AMinigamePlayer::ReadyPlayer(const FInputActionValue& Value)
{
	bIsReady = !bIsReady;
	Cast<AMinigameBase>(UGameplayStatics::GetGameMode(GetWorld()))->ReadyPlayer(PlayerNumber, bIsReady);
}
