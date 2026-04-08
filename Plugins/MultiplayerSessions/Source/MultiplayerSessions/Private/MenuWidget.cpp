#include "MenuWidget.h"

#include "Components/Button.h"

#include "MultiplayerSessionsSubsystem.h"


void UMenuWidget::MenuSetup(int32 InNumPublicConnections, FString InMatchType)
{
	NumPublicConnections = InNumPublicConnections;

	MatchType = InMatchType;

	AddToViewport();

	SetVisibility(ESlateVisibility::Visible);

	bIsFocusable = true;

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
	GEngine->AddOnScreenDebugMessage
	(
		-1,
		15.f,
		FColor::Yellow,
		FString(TEXT("Session Created Successfully!"))
	);

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
	UE_LOG(LogTemp, Warning, TEXT("UMenuWidget::JoinButtonClicked"));
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
