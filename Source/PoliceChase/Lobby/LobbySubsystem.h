#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Online.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "LobbySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyCreated, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionJoin, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbiesFound, bool, bSuccess, const FString&, Message);

USTRUCT(BlueprintType)
struct FLobbyInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FString LobbyName;

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 MaxPlayers = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	bool bIsPrivate = false;

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 SessionIndex = -1;
};

UCLASS(Blueprintable, BlueprintType)
class POLICECHASE_API ULobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	TArray<FLobbyInfo> GetLobbiesInfo() const;
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyCreated OnLobbyCreated;
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnSessionJoin OnSessionJoin;
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbiesFound OnLobbiesFound;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void CreateLobby(int32 MaxPlayers, bool bUseSteam);
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void FindLobbies();
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void JoinLobby(int32 LobbyIndex);

private:
	IOnlineSessionPtr SessionInterface;
	FName CurrentSessionName;
	FOnlineSessionSearchResult SessionToJoin;
	TArray<FOnlineSessionSearchResult> SearchResults;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;
	
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};