#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionsSubsystem.generated.h"


//
// Declaring our own custom delegates for the Menu class to bind callbacks to
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerCreateSessionCompleteSignature, bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerDestroySessionCompleteSignature, bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerStartSessionCompleteSignature, bool, bWasSuccessful);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMultiplayerJoinSessionCompleteSignature, EOnJoinSessionCompleteResult::Type Result);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMultiplayerFindSessionsCompleteSignature, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);


UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	//
	// To handle session functionality. The Menu class will call these
	//
	void CreateSession(int32 NumPublicConnections, FString MatchType);

	void CreateSessionInternal(int32 NumPublicConnections, FString MatchType);
	
	void FindSessions(int32 MaxSearchResults);

	void JoinSession(const FOnlineSessionSearchResult& SessionResult);

	void DestroySession();

	void StartSession();

	//
	// Our own custom delegates for the Menu class to bind callbacks to
	//

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnMultiplayerCreateSessionCompleteSignature MultiplayerOnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnMultiplayerDestroySessionCompleteSignature OnMultiplayerDestroySessionComplete;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnMultiplayerStartSessionCompleteSignature OnMultiplayerStartSessionComplete;

	FOnMultiplayerJoinSessionCompleteSignature OnMultiplayerJoinSessionComplete;

	FOnMultiplayerFindSessionsCompleteSignature OnMultiplayerFindSessions;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	bool bCreateSessionOnDestroy = false;

protected:

	//
	// Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	// These don't need to be called outside this class. 
	//

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	void OnFindSessionsComplete(bool bWasSuccessful);

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	//
	// To add to the Online Session Interface delegate list.
	// We'll bind our MultiplayerSessionsSubsystem internal callbacks to these.
	//

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;

	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;

	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;

	FDelegateHandle StartSessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;

};
