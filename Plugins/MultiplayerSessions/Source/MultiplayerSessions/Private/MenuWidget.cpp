#include "MenuWidget.h"

void UMenuWidget::MenuSetup()
{
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
}
