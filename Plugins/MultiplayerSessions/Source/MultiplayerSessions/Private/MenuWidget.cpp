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

void UMenuWidget::HostButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("UMenuWidget::HostButtonClicked"));

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
