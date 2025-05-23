/**************************************************************************************************
 * MinigameBase.cpp
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

#include "MinigameBase.h"

#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MinigameEnums.h"
#include "BasePointCounter.h"
#include "MinigameSessionSubsystem.h"
#include "TimerManager.h"
#include "MinigameCoreLogChannel.h"

// UI
#include "Widgets/MinigameUI.h"
#include "Widgets/PracticeModeUI.h"
#include "Widgets/SplitscreenUI.h"
#include "Components/OverlaySlot.h"
#include "Components/Overlay.h"

// Player Spawning
#include "Player/PlayerSpawn.h"
#include "Player/MinigamePlayer.h"
#include "MinigameCapture.h"
#include "Camera/CameraActor.h"
#include <Kismet/GameplayStatics.h>


#pragma region Initialization
AMinigameBase::AMinigameBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bIsPractice = true;
	SplitCameras.Init(nullptr, 4);
}

void AMinigameBase::StartPlay() 
{
	MinigameSessionSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UMinigameSessionSubsystem>();
	
	EndCamera = Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(this, ACameraActor::StaticClass()));

	NumPlayers = MinigameSessionSubsystem->GetPlayerCount();

#if WITH_EDITOR
	// For debug purposes only
	if (NumPlayers == 0) 
	{
		NumPlayers = 1;
	}
#endif

	FindMinigameCameras();

	FindPlayerSpawns();

	// Assign players to teams
	TeamsByPlayers = SplitIntoTeams();
	ReadyPlayers.Init(0, NumPlayers);

	if (UBasePointCounter* FoundCounter = Cast<UBasePointCounter>(GetComponentByClass(UBasePointCounter::StaticClass())))
	{
		PointCounter = FoundCounter;
	}
	else
	{
		PointCounter = NewObject<UBasePointCounter>(this, UBasePointCounter::StaticClass(), "Point Counter");
		PointCounter->RegisterComponent();
		
		AlertWarning("No point counter found. Creating default point counter.");
	}

	StartGame();

	Super::StartPlay();
}

void AMinigameBase::FindPlayerSpawns()
{
	// Get all player spawns
	TArray<AActor*> FoundSpawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerSpawn::StaticClass(), FoundSpawns);
	for (const auto FoundSpawn : FoundSpawns)
	{
		APlayerSpawn* Spawn = Cast<APlayerSpawn>(FoundSpawn);
		if (!Spawn) { continue; }

		// Should be safe to use values as index with no checks due to PlayerSpawn clamping
		if (PlayerSpawns[Spawn->NumPlayersRequired - 1][Spawn->Team][Spawn->Position] != nullptr)
		{
			AlertError(FString::Printf(
				TEXT("Minigame Base: Duplicate player spawn found for %d players, team %d, position %d."),
				Spawn->NumPlayersRequired, Spawn->Team, Spawn->Position));
			continue;
		}

		if (Spawn->bUseOnAnyPlayerCount)
		{
			for (int i = 0; i <= 3; ++i)
			{
				PlayerSpawns[i][Spawn->Team][Spawn->Position] = Spawn;
			}
		}
		else
		{
			PlayerSpawns[Spawn->NumPlayersRequired - 1][Spawn->Team][Spawn->Position] = Spawn;
		}
		
	}
}

void AMinigameBase::InitPoints() 
{
	// Indices are players
	if (TeamType == EMinigameType::MT_FFA) 
	{
		PointCounter->InitializePointCounter(NumPlayers, TeamsByPlayers);
	}
	// Indices are teams
	else 
	{
		PointCounter->InitializePointCounter(2, TeamsByPlayers);
	}
}

TArray<int> AMinigameBase::SplitIntoTeams() 
{
	/* Note: TeamType's int representation
	*  represents the number of players that
	*  the second team should have.
	*/

	// Ensure there are enough players
	if (NumPlayers <= static_cast<int>(TeamType)) 
	{
		AlertError("Not enough players for this game type!");
	}

	// Shuffle list of all players. Players are identified by their player number currently
	TArray<int> ShufflePlayers;
	ShufflePlayers.Init(0, NumPlayers);
	for (int i = 1; i < NumPlayers; ++i)
	{
		ShufflePlayers[i] = i;
	}

	if (TeamType == EMinigameType::MT_FFA)
	{
		return ShufflePlayers;
	}

	int NumShuffles = NumPlayers - 1;
	for (int i = 0; i < NumShuffles; ++i)
	{
		int SwapIdx = FMath::RandRange(i, NumShuffles);
		ShufflePlayers.Swap(i, SwapIdx);
	}

	TArray<int> TeamAssignment;
	TeamAssignment.Init(0, NumPlayers);

	// Assign players to teams randomly based on the shuffle
	for (int i = 0; i < static_cast<int>(TeamType) && i < NumPlayers; ++i)
	{
		TeamAssignment[ShufflePlayers[i]] = 1;
	}

	return TeamAssignment;
}

void AMinigameBase::FindMinigameCameras()
{
	TArray<AActor*> MinigameCameras;
	UGameplayStatics::GetAllActorsOfClass(this, AMinigameCapture::StaticClass(), MinigameCameras);
	for (const auto& Camera : MinigameCameras)
	{
		if (auto MinigameCamera = Cast<AMinigameCapture>(Camera))
		{
			AddCamera(MinigameCamera, MinigameCamera->GetCaptureNumber());
		}
	}
}

void AMinigameBase::AddCamera(AMinigameCapture* Camera, int CameraNumber)
{
	if (CameraNumber < 0)
	{
		AlertError("Attempted to add camera with an invalid camera number");
		Camera->EnableCamera(false);
		return;
	}
	if (CameraNumber >= GetNumCameraSplits())
	{
		// Camera does not have a valid camera split. No need to report this
		Camera->EnableCamera(false);
		return;
	}
	if (SplitCameras[CameraNumber])
	{
		AlertWarning("Two cameras have the same camera number. Ignoring newly added camera.");
		Camera->EnableCamera(false);
		return;
	}
	Camera->EnableCamera(true);
	SplitCameras[CameraNumber] = Camera;
}

AMinigameCapture* AMinigameBase::GetCamera(int PlayerNumber) const
{
	if (PlayerNumber < 0 || PlayerNumber >= 4)
	{
		AlertError("Attempted to get the camera associated with an invalid player number.\n");
		return nullptr;
	}
	if (!bUseSplitscreen)
	{
		return SplitCameras[0];
	}
	if (SplitBy == ESplitBy::PLAYERS)
	{
		return SplitCameras[PlayerNumber];
	}
	else
	{
		return SplitCameras[TeamsByPlayers[PlayerNumber]];
	}
}

#pragma endregion

#pragma region Game Start

AActor* AMinigameBase::GetPlayerSpawn(int Team, int Pos) const
{
	APlayerSpawn* PlayerSpawn = PlayerSpawns[NumPlayers - 1][Team][Pos];
	if (PlayerSpawn)
	{
		return PlayerSpawn;
	}
	// Valid player spawn wasn't found
	AlertError(FString::Printf(TEXT("Player spawn missing! NumPlayers: %d | Team %d | Position %d"), NumPlayers, Team, Pos));
	return nullptr;
}

void AMinigameBase::StartGame() 
{
	if (MinigameDuration == 0)
	{
		MinigameDuration = UE_DOUBLE_BIG_NUMBER;
	}

	TimeRemaining = MinigameDuration;
	TimeElapsed = 0;
	InitPoints();
	bIsInProgress = true;
	Players.Init(nullptr, NumPlayers);


	// Stores the position to spawn the next player on each team
	TArray<int> TeamNextStart;
	TeamNextStart.Init(0, 2);

	for (int i = 0; i < NumPlayers; i++)
	{
		// Get spawn location
		int Team{};
		if (TeamType == EMinigameType::MT_FFA)
		{
			Team = 0;
		}
		else
		{
			Team = TeamsByPlayers[i];
		}

		// Prevent crash if PlayerObjects is unassigned
		if (!PlayerObjects.IsValidIndex(Team) || PlayerObjects[Team] == nullptr) 
		{
			AlertError(FString::Printf(TEXT("No player object specified for team %d."), Team));
			continue;
		}

		AActor* Start = GetPlayerSpawn(Team, TeamNextStart[Team]++);
		FTransform StartPos{};
		AMinigamePlayer* Player{};
		if (Start != nullptr)
		{
			// Spawn player
			StartPos = FTransform{ Start->GetActorRotation(), Start->GetActorLocation(), FVector{1,1,1} };
		}

		Player = GetWorld()->SpawnActorDeferred<AMinigamePlayer>(PlayerObjects[Team], StartPos);

		Player->Team = (TeamType == EMinigameType::MT_FFA) ? i : TeamsByPlayers[i];
		APlayerController* Controller = UGameplayStatics::GetPlayerControllerFromID(GetWorld(), i);

		if (Controller)
		{
			Controller->Possess(Player);
			Controller->SetViewTarget(EndCamera);
		}

		Player->Team = (TeamType == EMinigameType::MT_FFA) ? i : TeamsByPlayers[i];
		Player->PlayerNumber = i;
		Player->Minigame = this;

		UGameplayStatics::FinishSpawningActor(Player, StartPos);

		Players[i] = Player;
	}

	ClearMinigameUI();
	DrawMinigameUI();
	OnGameStart.Broadcast();
}

int AMinigameBase::GetNumCameraSplits() const
{
	if (!bUseSplitscreen)
	{
		return 1;
	}
	// Each player gets a split
	if (SplitBy == ESplitBy::PLAYERS || TeamType == EMinigameType::MT_FFA)
	{
		return NumPlayers;
	}
	// Each team gets a split. Team based game modes currently only have 2 teams
	if (SplitBy == ESplitBy::TEAMS)
	{
		return 2;
	}
	// No split - just 1 camera
	return 1;
}

void AMinigameBase::DrawMinigameUI()
{
	if (DebugHideUI) { return; }
	
	// Check if splitscreen class is provided
	if (SplitscreenUIClasses.Num() == 0)
	{
		AlertError("No splitscreen UI provided. Please provide at least a 1-view splitscreen UI. Could not spawn minigame UI.");
		return;
	}

	// Choose which splitscreen UI to use
	TSubclassOf<USplitscreenUI> SplitscreenClass{ SplitscreenUIClasses[0] };
	if (bUseSplitscreen)
	{
		int NumSplits = GetNumCameraSplits();
		if (SplitscreenUIClasses.Num() < NumSplits)
		{
			AlertError("No splitscreen UI provided for the specified number of teams. Could not spawn minigame UI.");
		}
		else
		{
			SplitscreenClass = SplitscreenUIClasses[NumSplits - 1];
		}

	}

	if (!SplitscreenClass)
	{
		AlertError("No splitscreen UI provided!");
		return;
	}

	// Spawn splitscreen UI
	USplitscreenUI* SplitscreenUI = static_cast<USplitscreenUI*>(UUserWidget::CreateWidgetInstance(*GetWorld(), SplitscreenClass, FName(TEXT("Splitscreen UI"))));
	CurrentMinigameUI = SplitscreenUI;
	SplitscreenUI->AddToViewport();
	int NumSplits = SplitscreenUI->GetNumSplits();

	// Spawn minigame UI in each split
	if (MinigameUIClass != nullptr)
	{
		for (int i = 0; i < NumSplits; ++i)
		{
			UMinigameUI* MinigameUI = Cast<UMinigameUI>(UUserWidget::CreateWidgetInstance(*GetWorld(),
				MinigameUIClass, FName(FString::Printf(TEXT("Minigame UI %d"), i))));
			// UI names must be unique!

			// Spawn minigame UI
			MinigameUI->AddToViewport();
			MinigameUI->SetMinigame(this);
			MinigameUI->SetAssignedSplit(i);
			MinigameUI->StartUI();

			// Add it to the split
			UOverlay* Split{ SplitscreenUI->GetSplitPanel(i) };
			UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Split->AddChild(MinigameUI));

			// Minigame UI should fill entire provided split
			OverlaySlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
			OverlaySlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
		}
	}

	if (PracticeUIClass == nullptr)
	{
		AlertError("No practice UI provided!");
		return;
	}

	// Spawn practice UI
	if (bIsPractice)
	{
		if (!PracticeUI)
		{
			PracticeUI = Cast<UPracticeModeUI>(UUserWidget::CreateWidgetInstance(*GetWorld(), PracticeUIClass, FName(TEXT("Practice Mode UI"))));
			PracticeUI->AddToViewport();
		}

		UPanelWidget* PreviewWindow = PracticeUI->GetPreviewWindow();
		PreviewWindow->AddChild(SplitscreenUI);
	}
}


void AMinigameBase::ClearMinigameUI()
{
	if (CurrentMinigameUI)
	{
		CurrentMinigameUI->RemoveFromParent();
		CurrentMinigameUI = nullptr;
	}
	if (!bIsPractice && PracticeUI)
	{
		PracticeUI->RemoveFromParent();
		PracticeUI = nullptr;
	}
}
#pragma endregion

#pragma region Game End

void AMinigameBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsInProgress) { return; }

	TimeRemaining -= DeltaTime;
	TimeElapsed += DeltaTime;
	if (TimeRemaining <= 0) 
	{
		EndGame();
	}
}


void AMinigameBase::StartReset()
{
	// Reset game after brief delay for reset effects
	GetWorldTimerManager().SetTimer(ResetTimerHandle,
		[=]() { ResetLevel(); },
		ResetDelay, false);
}
void AMinigameBase::ResetLevel() 
{
	OnGameReset.Broadcast();

	Super::ResetLevel();

	ClearMinigameUI();

	if (bIsPractice && PracticeUI)
	{
		PracticeUI->EndResetEffect();
	}

	// Play level sequence for flythrough
	if (!bIsPractice && LevelSequence)
	{
		ALevelSequenceActor* SequenceActor;
		SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this, LevelSequence, {}, SequenceActor);
		SequencePlayer->Play();

		TScriptDelegate FinalizeStartEvent{};
		FinalizeStartEvent.BindUFunction(this, FName("StartGame"));
		SequencePlayer->OnFinished.Add(FinalizeStartEvent);
	}
	else
	{
		StartGame();
	}
}

void AMinigameBase::EndGame() 
{
	// Ending the game should generally transition back to practice mode if we're in practice mode
	EndGame(bIsPractice);
}

void AMinigameBase::EndGame(bool bTransitionToPractice, bool bOverrideIfInProgress)
{
	if (!bIsInProgress && !bOverrideIfInProgress) { return; }
	bIsInProgress = false;

	// Practice mode could end to transition back to practice mode, 
	// or it could end to transition into the main game
	if (bIsPractice)
	{
		StartReset();

		if (bTransitionToPractice && PracticeUI)
		{
			PracticeUI->PlayResetEffect();
		}
	}
	else
	{
		ProcessMinigameEnd();
	}

	OnGameEnd.Broadcast();
}
void AMinigameBase::ProcessMinigameEnd()
{
	DrawEndUI();

	MinigameSessionSubsystem->SetMinigameResults(PointCounter->GetStandings());
	
	FTimerHandle EndHandle;
	GetWorldTimerManager().SetTimer(EndHandle, this, &AMinigameBase::ChangeLevel, TransitionDelay, false);
}
void AMinigameBase::DrawEndUI()
{
	if (EndUIClass)
	{
		UUserWidget* EndUI = UUserWidget::CreateWidgetInstance(*GetWorld(), EndUIClass, FName(TEXT("End UI")));
		if (EndUI)
		{
			EndUI->AddToViewport();
		}
	}
}

void AMinigameBase::ChangeLevel()
{
	FLoadLevelParams LoadLevelParams;
	LoadLevelParams.LevelToOpen = MinigameSessionSubsystem->GetMinigameReturnLevel();

	// Loop if the minigame wasn't initiated by the board
	if (LoadLevelParams.LevelToOpen.IsNull())
	{
		LoadLevelParams.LevelToOpen = GetLevel();
	}
	TransitionToLevel(LoadLevelParams);
}


#pragma endregion

#pragma region Player Readying

void AMinigameBase::ReadyPlayer(int Player, bool State)
{
	if (!bIsPractice) { return; }
	ReadyPlayers[Player] = true;
	for (int i = 0; i < ReadyPlayers.Num(); i++) 
	{
		if (ReadyPlayers[i] == false) 
		{
			return;
		}
	}
	EndPracticeMode();
}

void AMinigameBase::EndPracticeMode()
{
	OnPracticeModeEnd.Broadcast();

	EndGame(false, true);
	bIsPractice = false;

	PlayTransitionEffects();
}

int AMinigameBase::CheckReady(int Player) const 
{
	return ReadyPlayers[Player];
}

#pragma endregion


AMinigamePlayer* AMinigameBase::GetPlayer(int PlayerNum) const
{
	if (PlayerNum < Players.Num() && PlayerNum >= 0)
	{
		return Players[PlayerNum];
	}
	return nullptr;
}

// TODO: Consider define our own log category rather than using LogTemp
void AMinigameBase::AlertWarning(const FString& Text) const
{
	UE_LOG(LogMinigameCore, Warning, TEXT("MinigameBase: %s"), *Text);
	GEngine->AddOnScreenDebugMessage(-1, 50, FColor::Yellow, FString::Printf(TEXT("MinigameBase: %s"), *Text));
}
void AMinigameBase::AlertError(const FString& Text) const
{
	UE_LOG(LogMinigameCore, Error, TEXT("MinigameBase: %s"), *Text);
	GEngine->AddOnScreenDebugMessage(-1, 50, FColor::Red, FString::Printf(TEXT("MinigameBase: %s"), *Text));
}