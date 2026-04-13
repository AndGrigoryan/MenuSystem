#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

#include "GameFramework/PlayerState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (IsValid(GameState))
	{
		int32 numberOfPlayers = GameState->PlayerArray.Num();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage
			(
				1,
				60.f,
				FColor::Yellow,
				FString::Printf(TEXT("Players in game: %d"), numberOfPlayers)
			);

			APlayerState* playerState = NewPlayer->GetPlayerState<APlayerState>();

			if (IsValid(playerState))
			{
				FString playerName = playerState->GetPlayerName();

				GEngine->AddOnScreenDebugMessage
				(
					-1,
					60.f,
					FColor::Cyan,
					FString::Printf(TEXT("%s has joined the game!"), *playerName)
				);
			}
		}
	}

}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	APlayerState* playerState = Exiting->GetPlayerState<APlayerState>();

	if (IsValid(playerState) && IsValid(GEngine))
	{
		int32 numberOfPlayers = GameState->PlayerArray.Num();

		GEngine->AddOnScreenDebugMessage
		(
			1,
			60.f,
			FColor::Yellow,
			FString::Printf(TEXT("Players in game: %d"), numberOfPlayers - 1)
		);

		FString playerName = playerState->GetPlayerName();

		GEngine->AddOnScreenDebugMessage
		(
			-1,
			60.f,
			FColor::Cyan,
			FString::Printf(TEXT("%s has exited the game!"), *playerName)
		);
	}
}
