#include "MenuWidget.h"

#include "Components/Button.h"

#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSessionSettings.h"

#include "OnlineSubsystem.h"

#include "Kismet/KismetSystemLibrary.h"


UMenuWidget::UMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void UMenuWidget::MenuSetup
(
	TSoftObjectPtr<UWorld> InLobbyMap,
	int32 InNumPublicConnections, 
	FString InMatchType
)
{
	PathToLobby = FString(*FPackageName::ObjectPathToPackageName(InLobbyMap.ToString()));
	

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

		HostButton->SetIsEnabled(true);

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
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenuWidget::OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (!bWasSuccessful || SessionResults.IsEmpty())
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenuWidget::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);

	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType, PathToLobby);
	}
}

void UMenuWidget::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);

	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
		MultiplayerSessionsSubsystem->SetSelectedMatchType(MatchType);
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
