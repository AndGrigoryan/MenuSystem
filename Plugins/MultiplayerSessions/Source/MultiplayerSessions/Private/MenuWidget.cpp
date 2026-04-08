#include "MenuWidget.h"

#include "Components/Button.h"

#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSessionSettings.h"

#include "OnlineSubsystem.h"


UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UE_LOG(LogTemp, Error, TEXT("UMenuWidget::UMenuWidget"));
}

void UMenuWidget::MenuSetup(int32 InNumPublicConnections, FString InMatchType)
{
	NumPublicConnections = InNumPublicConnections;

	MatchType = InMatchType;

	AddToViewport();

	SetVisibility(ESlateVisibility::Visible);

	SetIsFocusable(true);
	

	UWorld* world = GetWorld();

	if (!IsValid(world))
	{
		return;
	}

	APlayerController* playerController = world->GetFirstPlayerController();

	if (IsValid(playerController))
	{
		FInputModeUIOnly inputModeUIOnlyData;
		inputModeUIOnlyData.SetWidgetToFocus(TakeWidget());
		inputModeUIOnlyData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		playerController->SetInputMode(inputModeUIOnlyData);
		playerController->SetShowMouseCursor(true);
	}

	UGameInstance* gameInstance = GetGameInstance();

	if (IsValid(gameInstance))
	{
		MultiplayerSessionsSubsystem = gameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &UMenuWidget::OnCreateSession);

		MultiplayerSessionsSubsystem->OnMultiplayerDestroySessionComplete.AddDynamic(this, &UMenuWidget::OnDestroySession);

		MultiplayerSessionsSubsystem->OnMultiplayerStartSessionComplete.AddDynamic(this, &UMenuWidget::OnStartSession);

		MultiplayerSessionsSubsystem->OnMultiplayerJoinSessionComplete.AddUObject(this, &UMenuWidget::OnJoinSession);

		MultiplayerSessionsSubsystem->OnMultiplayerFindSessionsComplete.AddUObject(this, &UMenuWidget::OnFindSession);
	}

}

bool UMenuWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (IsValid(HostButton))
	{
		HostButton->OnClicked.AddDynamic(this, &UMenuWidget::HostButtonClicked);
	}

	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.AddDynamic(this, &UMenuWidget::JoinButtonClicked);
	}

	return true;
}

void UMenuWidget::NativeDestruct()
{
	MenuTearDown();

	Super::NativeDestruct();
}

void UMenuWidget::OnCreateSession(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage
			(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("Failed to Create Session!"))
			);
		}
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage
		(
			-1,
			15.f,
			FColor::Yellow,
			FString(TEXT("Session Created Successfully!"))
		);
	}

}

void UMenuWidget::OnDestroySession(bool bWasSuccessful)
{
}

void UMenuWidget::OnStartSession(bool bWasSuccessful)
{
}

void UMenuWidget::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
	if (subsystem)
	{
		IOnlineSessionPtr sessionInterface = subsystem->GetSessionInterface();

		if (sessionInterface.IsValid())
		{
			FString address;
			
			sessionInterface->GetResolvedConnectString(NAME_GameSession, address);

			APlayerController* playerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (playerController)
			{
				playerController->ClientTravel(address, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UMenuWidget::OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (!IsValid(MultiplayerSessionsSubsystem))
	{
		return;
	}

	for (const auto& res : SessionResults)
	{
		FString id = res.GetSessionIdStr();
		FString user = res.Session.OwningUserName;

		FString settingsValue;
		res.Session.SessionSettings.Get(FName("MatchType"), settingsValue);

		if (settingsValue == MatchType)
		{
			MultiplayerSessionsSubsystem->JoinSession(res);
			return;
		}
	}
}

void UMenuWidget::HostButtonClicked()
{
	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenuWidget::JoinButtonClicked()
{
	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void UMenuWidget::MenuTearDown()
{
	RemoveFromParent();

	UWorld* world = GetWorld();

	if (!IsValid(world))
	{
		return;
	}

	APlayerController* playerController = world->GetFirstPlayerController();

	if (IsValid(playerController))
	{
		FInputModeGameOnly inputModeGameOnlyData;

		playerController->SetInputMode(inputModeGameOnlyData);
		playerController->SetShowMouseCursor(false);
	}

}
