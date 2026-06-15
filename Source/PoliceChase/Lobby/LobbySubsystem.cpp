#include "Lobby/LobbySubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

void ULobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    
    // On ne force le sous-système NULL QUE si aucun autre (comme Steam) n'est configuré/disponible
    if (!OnlineSubsystem)
    {
        UE_LOG(LogOnline, Warning, TEXT("Online Subsystem not found - using NULL"));
        OnlineSubsystem = IOnlineSubsystem::Get(NULL_SUBSYSTEM);
    }
    
    if (OnlineSubsystem)
    {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &ULobbySubsystem::OnCreateSessionComplete);
            SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &ULobbySubsystem::OnFindSessionsComplete);
            SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &ULobbySubsystem::OnJoinSessionComplete);
        }
    }
}

void ULobbySubsystem::CreateLobby(int32 MaxPlayers, bool bUseSteam)
{
    if (!SessionInterface.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("CreateLobby: SessionInterface not valid"));
        OnLobbyCreated.Broadcast(false, TEXT("SessionInterface not valid"));
        return;
    }
    
    FOnlineSessionSettings SessionSettings;
    SessionSettings.NumPublicConnections = MaxPlayers;
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bUsesPresence = true;

    // Utilisation réelle du paramètre bUseSteam ou fallback LAN si on est sur le subsystem NULL
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    bool bIsNULL = (Subsystem && Subsystem->GetSubsystemName() == NULL_SUBSYSTEM);
    
    // Si on demande Steam mais qu'on est en NULL, ou si bUseSteam est faux -> on passe en LAN
    SessionSettings.bIsLANMatch = !bUseSteam || bIsNULL; 
    
    // CORRECTION : On utilise le nom standard global d'Unreal pour la session de jeu
    bool bSuccess = SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
    if (!bSuccess)
    {
        OnLobbyCreated.Broadcast(false, TEXT("Failed to create session"));
    }
}

void ULobbySubsystem::FindLobbies()
{
    if (!SessionInterface.IsValid())
    {
        OnLobbiesFound.Broadcast(false, TEXT("SessionInterface not valid"));
        return;
    }
    
    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->MaxSearchResults = 100;

    // Recherche en LAN si le sous-système actuel est NULL
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    bool bIsLAN = (Subsystem && Subsystem->GetSubsystemName() == NULL_SUBSYSTEM);
    SessionSearch->bIsLanQuery = bIsLAN; 
    
    // Recherche de présence requise pour Steam / NULL standard
    SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
    
    SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

TArray<FLobbyInfo> ULobbySubsystem::GetLobbiesInfo() const
{
    TArray<FLobbyInfo> LobbyInfos;
    
    if (SearchResults.Num() == 0) return LobbyInfos;

    for (int32 i = 0; i < SearchResults.Num(); ++i)
    {
        const FOnlineSessionSearchResult& Result = SearchResults[i];
        FLobbyInfo Info;

        Info.LobbyName = Result.Session.OwningUserName;
        if (Info.LobbyName.IsEmpty())
        {
            Info.LobbyName = FString::Printf(TEXT("Lobby %d"), i + 1);
        }

        Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
        Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections; 
        
        Info.bIsPrivate = false;
        Info.SessionIndex = i;

        LobbyInfos.Add(Info);
    }

    return LobbyInfos;
}

void ULobbySubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("Lobby créée avec succès !"));
        OnLobbyCreated.Broadcast(true, TEXT("Lobby created"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Erreur création lobby"));
        OnLobbyCreated.Broadcast(false, TEXT("Failed to create lobby"));
    }
}

void ULobbySubsystem::Deinitialize()
{
    Super::Deinitialize();
    SessionInterface = nullptr;
}

void ULobbySubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (!bWasSuccessful || !SessionSearch.IsValid())
    {
        OnLobbiesFound.Broadcast(false, TEXT("Failed to find sessions"));
        return;
    }
    
    SearchResults = SessionSearch->SearchResults;
    
    UE_LOG(LogTemp, Warning, TEXT("Found %d lobbies"), SearchResults.Num());
    OnLobbiesFound.Broadcast(true, TEXT("Lobbies found"));
}

void ULobbySubsystem::JoinLobby(int32 LobbyIndex)
{
    if (!SessionInterface.IsValid() || SearchResults.Num() == 0)
    {
        OnSessionJoin.Broadcast(false, TEXT("No lobbies available"));
        return;
    }
    
    if (LobbyIndex >= SearchResults.Num())
    {
        OnSessionJoin.Broadcast(false, TEXT("Invalid lobby index"));
        return;
    }
    
    SessionToJoin = SearchResults[LobbyIndex];
    
    // CORRECTION : On rejoint en utilisant le même NAME_GameSession localement
    SessionInterface->JoinSession(0, NAME_GameSession, SessionToJoin);
}

void ULobbySubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("Lobby rejointe avec succès !"));
        OnSessionJoin.Broadcast(true, TEXT("Joined successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Erreur rejoindre lobby"));
        OnSessionJoin.Broadcast(false, TEXT("Failed to join"));
    }
}