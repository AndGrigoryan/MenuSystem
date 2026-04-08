#include "MultiplayerSessionsSubsystem.h"

#include "Kismet/KismetSystemLibrary.h"

#include "OnlineSubsystem.h"

#include "OnlineSessionSettings.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
	IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
	if (subsystem)
	{
		SessionInterface = subsystem->GetSessionInterface();
	}

	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionsSubsystem::OnCreateSessionComplete);

	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionsSubsystem::OnDestroySessionComplete);

	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMultiplayerSessionsSubsystem::OnFindSessionsComplete);

	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionsSubsystem::OnJoinSessionComplete);

	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionsSubsystem::OnStartSessionComplete);

}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	auto existingSession = SessionInterface->GetNamedSession(NAME_GameSession);

	if (existingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;

		if (DestroySessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			DestroySessionCompleteDelegateHandle.Reset();
		}

		DestroySessionCompleteDelegateHandle =
			SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);


		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	CreateSessionInternal(NumPublicConnections, MatchType);
}

void UMultiplayerSessionsSubsystem::CreateSessionInternal(int32 NumPublicConnections, FString MatchType)
{
	if (!IsValid(GetWorld()))
	{
		return;
	}

	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	// Store the delegate in a FDelegateHandle so we can later remove it from the delegate list
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());

	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* localPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (!localPlayer || !localPlayer->GetPreferredUniqueNetId().IsValid())
	{
		UKismetSystemLibrary::PrintString
		(
			GetWorld(),
			FString(TEXT("Invalid local player or localPlayer->GetPreferredUniqueNetId()")),
			true,
			true,
			FLinearColor::Red
		);
		return;
	}

	const bool bcreateSessionStarted = SessionInterface->CreateSession(*localPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings);

	if (!bcreateSessionStarted)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Broadcast  our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
}

void UMultiplayerSessionsSubsystem::StartSession()
{
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UWorld* world = GetWorld();

	if (!IsValid(world))
	{
		return;
	}

	if (SessionInterface.IsValid() && CreateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);

	UKismetSystemLibrary::PrintString
	(
		world,
		FString::Printf(TEXT("AMenuSystemCharacter::OnCreateSessionComplete %s success = %d"), *SessionName.ToString(), bWasSuccessful),
		true,
		true,
		bWasSuccessful ? FLinearColor::Green : FLinearColor::Red
	);

	if (bWasSuccessful)
	{
		world->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"), true);
	}
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
