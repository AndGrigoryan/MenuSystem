#include "MultiplayerSessionsSubsystem.h"

#include "Kismet/KismetSystemLibrary.h"

#include "OnlineSubsystem.h"

#include "OnlineSessionSettings.h"

#include "Online/OnlineSessionNames.h"

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

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString InMatchType)
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

	CreateSessionInternal(NumPublicConnections, InMatchType);
}

void UMultiplayerSessionsSubsystem::CreateSessionInternal(int32 NumPublicConnections, FString InMatchType)
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
	LastSessionSettings->Set(FName("MatchType"), InMatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;

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
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);


	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());

	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

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

	const bool bSearchSessionsSuccessful = SessionInterface->FindSessions(*localPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());

	if (!bSearchSessionsSuccessful)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();

		OnMultiplayerFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		OnMultiplayerJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

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

	bool bjoinSessionStarted = SessionInterface->JoinSession(*localPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult);

	if (!bjoinSessionStarted)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		OnMultiplayerJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}

}

void UMultiplayerSessionsSubsystem::DestroySession()
{
}

void UMultiplayerSessionsSubsystem::StartSession()
{
}

void UMultiplayerSessionsSubsystem::SetSelectedMatchType(const FString& InMatchType)
{
	MatchType = InMatchType;
}

FString UMultiplayerSessionsSubsystem::GetSelectedMatchType() const
{
	return MatchType;
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
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	if (LastSessionSearch->SearchResults.IsEmpty())
	{
		OnMultiplayerFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	OnMultiplayerFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);

	for (const auto& res : LastSessionSearch->SearchResults)
	{
		FString id = res.GetSessionIdStr();
		FString user = res.Session.OwningUserName;

		FString settingsValue;
		res.Session.SessionSettings.Get(FName("MatchType"), settingsValue);

		if (settingsValue == MatchType)
		{
			JoinSession(res);
			return;
		}
	}
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		return;
	}

	OnMultiplayerJoinSessionComplete.Broadcast(Result);

	if (SessionInterface.IsValid())
	{
		FString address;

		SessionInterface->GetResolvedConnectString(NAME_GameSession, address);

		APlayerController* playerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (playerController)
		{
			playerController->ClientTravel(address, ETravelType::TRAVEL_Absolute);
		}
	}
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
