// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"

#include "Engine/LocalPlayer.h"

#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "InputActionValue.h"

#include "MenuSystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"


#include "Kismet/KismetSystemLibrary.h"

AMenuSystemCharacter::AMenuSystemCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject
	(
		this, &AMenuSystemCharacter::OnCreateSessionComplete
	);

	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject
	(
		this, &AMenuSystemCharacter::OnDestroySessionComplete
	);

	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject
	(
		this, &AMenuSystemCharacter::OnFindSessionsComplete
	);

	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject
	(
		this, &AMenuSystemCharacter::OnJoinSessionComplete
	);


	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();

	if (onlineSubsystem)
	{
		OnlineSessionInterface = onlineSubsystem->GetSessionInterface();


		if (GetWorld())
		{
			UKismetSystemLibrary::PrintString
			(
				GetWorld(),
				FString::Printf(TEXT("Found subsystem %s"), *onlineSubsystem->GetSubsystemName().ToString()),
				true,
				true,
				FLinearColor::Blue
			);
		}

	}

}

void AMenuSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Look);
	}
	else
	{
		UE_LOG(LogMenuSystem, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMenuSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMenuSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMenuSystemCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMenuSystemCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMenuSystemCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMenuSystemCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMenuSystemCharacter::CreateGameSession()
{
	// Called when pressing the 1 key
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	auto existingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);

	if (existingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;

		if (DestroySessionCompleteDelegateHandle.IsValid())
		{
			OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			DestroySessionCompleteDelegateHandle.Reset();
		}

		DestroySessionCompleteDelegateHandle =
			OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);


		OnlineSessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	CreateSessionInternal();
}

void AMenuSystemCharacter::JoinGameSession()
{
	// Find game sessions
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	FindSessionsCompleteDelegateHandle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* localPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	OnlineSessionInterface->FindSessions(*localPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());

}

void AMenuSystemCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UWorld* world = GetWorld();

	if (!IsValid(world))
	{
		return;
	}

	if (OnlineSessionInterface.IsValid() && CreateSessionCompleteDelegateHandle.IsValid())
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

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
		world->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
	}

}

void AMenuSystemCharacter::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (!IsValid(GetWorld()))
	{
		return;
	}

	if (DestroySessionCompleteDelegateHandle.IsValid())
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		DestroySessionCompleteDelegateHandle.Reset();
	}

	UKismetSystemLibrary::PrintString
	(
		GetWorld(),
		FString::Printf(TEXT("AMenuSystemCharacter::OnDestroySessionComplete %s success = %d"), *SessionName.ToString(), bWasSuccessful),
		true,
		true,
		bWasSuccessful ? FLinearColor::Green : FLinearColor::Red
	);

	if (bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSessionInternal();
	}
}

void AMenuSystemCharacter::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!IsValid(GetWorld()))
	{
		return;
	}

	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	for (const auto& res : SessionSearch->SearchResults)
	{
		FString id = res.GetSessionIdStr();
		FString user = res.Session.OwningUserName;

		FString matchType;
		res.Session.SessionSettings.Get(FName("MatchType"), matchType);

		UKismetSystemLibrary::PrintString
		(
			GetWorld(),
			FString::Printf(TEXT("Id: %s, User: %s"), *id, *user)
		);

		if (matchType == FString("FreeForAll"))
		{
			UKismetSystemLibrary::PrintString
			(
				GetWorld(),
				FString::Printf(TEXT("Joining Match Type: %s"), *matchType)
			);

			if (JoinSessionCompleteDelegateHandle.IsValid())
			{
				OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
				JoinSessionCompleteDelegateHandle.Reset();
			}

			JoinSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

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

			OnlineSessionInterface->JoinSession(*localPlayer->GetPreferredUniqueNetId(), NAME_GameSession, res);
			break;
		}
	}
}

void AMenuSystemCharacter::CreateSessionInternal()
{
	if (!IsValid(GetWorld()))
	{
		return;
	}

	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	CreateSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	TSharedPtr<FOnlineSessionSettings> sessionSettings = MakeShareable(new FOnlineSessionSettings());
	sessionSettings->bIsLANMatch = false;
	sessionSettings->NumPublicConnections = 4;
	sessionSettings->bAllowJoinInProgress = true;
	sessionSettings->bAllowJoinViaPresence = true;
	sessionSettings->bShouldAdvertise = true;
	sessionSettings->bUsesPresence = true;
	sessionSettings->bUseLobbiesIfAvailable = true;
	sessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

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


	OnlineSessionInterface->CreateSession(*localPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *sessionSettings);
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!OnlineSessionInterface.IsValid() ||
		!IsValid(GetWorld()))
	{
		return;
	}

	if (JoinSessionCompleteDelegateHandle.IsValid())
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		return;
	}

	FString address;

	if (OnlineSessionInterface->GetResolvedConnectString(SessionName, address))
	{
		UKismetSystemLibrary::PrintString
		(
			GetWorld(),
			FString::Printf(TEXT("Connect string: %s"), *address),
			true,
			true,
			FLinearColor::Yellow
		);

		APlayerController* playerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (playerController)
		{
			playerController->ClientTravel(address, ETravelType::TRAVEL_Absolute);
		}
	}
}
