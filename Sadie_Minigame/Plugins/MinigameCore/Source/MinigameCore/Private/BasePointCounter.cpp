/**************************************************************************************************
 * BasePointCounter.cpp
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

#include "BasePointCounter.h"

#include "MinigameCoreLogChannel.h"
#include "MinigameStanding.h"

UBasePointCounter::UBasePointCounter()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBasePointCounter::AddPoints(int Team, int Amount)
{
	if (0 <= Team && Team < TeamPoints.Num())
	{
		TeamPoints[Team] += Amount;
		if (Amount != 0)
		{
			OnPointsChanged.Broadcast(Team, TeamPoints[Team]);
		}
	}
	else
	{
		UE_LOG(LogMinigameCore, Error, TEXT("PointCounter: Invalid team number."));
	}
}


void UBasePointCounter::SetPoints(int Team, int Amount)
{
	if (0 <= Team && Team < TeamPoints.Num())
	{
		int original = TeamPoints[Team];
		TeamPoints[Team] = Amount;
		if (original != Amount)
		{
			OnPointsChanged.Broadcast(Team, TeamPoints[Team]);
		}
	}
	else
	{
		UE_LOG(LogMinigameCore, Error, TEXT("PointCounter: Invalid team number."));
	}
}

int UBasePointCounter::GetPoints(int Team) const
{
	if (0 <= Team && Team < TeamPoints.Num())
	{
		return TeamPoints[Team];
	}
	else
	{
		UE_LOG(LogMinigameCore, Error, TEXT("PointCounter: Invalid team number."));
		return -999;
	}
}

void UBasePointCounter::InitializePointCounter(int NumTeams, TArray<int> TeamAssignment)
{
	TeamPoints.Init(0, NumTeams);
	PlayersToTeams = TeamAssignment;
}

int UBasePointCounter::GetPointsByPlayer(int Player) const
{
	return TeamPoints[PlayersToTeams[Player]];
}

TArray<FMinigameStanding> UBasePointCounter::GetStandings() const
{
	int Num = PlayersToTeams.Num();
	TArray<FMinigameStanding> Res;
	Res.Init({ 0,0 }, Num);

	for (int i = 0; i < Num; i++)
	{
		Res[i].Player = i;
	}

	// Insertion sort
	// Sort the array of player indices such that their corresponding points
	// are in descending order.
	for (int i = 1; i < Num; i++)
	{
		for (int j = i; j > 0; j--)
		{
			// Check if adjacent elements are inverted
			if (GetPointsByPlayer(Res[i].Player) < GetPointsByPlayer(Res[i].Player))
			{
				// Swap elements
				int Temp{ Res[i].Player };
				Res[j].Player = Res[j - 1].Player;
				Res[j - 1].Player = Temp;
			}
		}
	}

	// Set standings based on ordered player indices
	for (int i = 0; i < Num; i++) {
		// Check if there is a tie. Tied players should have the same ranking
		if (i > 0 && GetPointsByPlayer(Res[i - 1].Player) == GetPointsByPlayer(Res[i].Player))
		{
			Res[i].Place = Res[i - 1].Place;
		}
		else
		{
			Res[i].Place = i + 1;
		}
	}

	FString Log{};
	for (const auto& Standing : Res)
	{
		Log += FString::Printf(TEXT("Player: %d | Place: %d\n"), Standing.Player, Standing.Place);
	}
	UE_LOG(LogMinigameCore, Log, TEXT("%s"), *Log);

	return Res;
}


