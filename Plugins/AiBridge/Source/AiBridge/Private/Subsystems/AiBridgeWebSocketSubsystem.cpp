// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeWebSocketSubsystem.h"

#include "WebSocketsModule.h"
#include "Authentication/JwtAuthenticationService.h"

void UAiBridgeWebSocketSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    AuthService = NewObject<UJwtAuthenticationService>(this);
    AuthService->Initialize(ApiBaseUrl);
    
    InitializeConnectionSequence();
    
    UE_LOG(LogTemp, Log, TEXT("UAiBridgeWebSocketSubsystem Initialized"));
}

void UAiBridgeWebSocketSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("UAiBridgeWebSocketSubsystem Deinitialized"));
    Disconnect();
    Super::Deinitialize();
}

void UAiBridgeWebSocketSubsystem::Connect(const FString& Url)
{
    if (Socket.IsValid())
    {
        Socket->Close();
        Socket.Reset();
    }

    Socket = FWebSocketsModule::Get().CreateWebSocket(Url);

    Socket->OnConnected().AddLambda([this]()
    {
        UE_LOG(LogTemp, Log, TEXT("WebSocket Connected"));
        OnConnected.Broadcast();
    });

    Socket->OnConnectionError().AddLambda([](const FString& Error)
    {
        UE_LOG(LogTemp, Error, TEXT("WebSocket Error: %s"), *Error);
    });

    Socket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean)
    {
        UE_LOG(LogTemp, Warning, TEXT("WebSocket Closed: %s"), *Reason);
        OnDisconnected.Broadcast();
    });

    Socket->OnMessage().AddLambda([this](const FString& Message)
    {
        OnTextMessage.Broadcast(Message);
    });

    Socket->OnRawMessage().AddLambda(
        [this](const void* Data, SIZE_T Size, SIZE_T BytesRemaining)
        {
            TArray<uint8> Buffer;
            Buffer.SetNumUninitialized(Size);
            FMemory::Memcpy(Buffer.GetData(), Data, Size);

            OnBinaryMessage.Broadcast(Buffer);
        });

    Socket->Connect();
}

void UAiBridgeWebSocketSubsystem::Disconnect()
{
    if (Socket.IsValid())
    {
        Socket->Close();
        Socket.Reset();
    }
}

void UAiBridgeWebSocketSubsystem::SendText(const FString& Message)
{
    if (Socket.IsValid() && Socket->IsConnected())
    {
        Socket->Send(Message);
    }
}

void UAiBridgeWebSocketSubsystem::SendBinary(const TArray<uint8>& Data)
{
    if (Socket.IsValid() && Socket->IsConnected())
    {
        Socket->Send(Data.GetData(), Data.Num(), true);
    }
}

bool UAiBridgeWebSocketSubsystem::IsConnected() const
{
    return Socket.IsValid() && Socket->IsConnected();
}

void UAiBridgeWebSocketSubsystem::InitializeConnectionSequence()
{
    if (sendWakeUpCall)
    {
        SendWakeUpCallAsync();
    }
    PreFetchJwtToken();
}

void UAiBridgeWebSocketSubsystem::SendWakeUpCallAsync() const
{
    bool bEnableVerboseLogging = true;
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UnifiedWebSocket] No World available"));
        return;
    }
    
    FTimerHandle TimerHandle;

    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        [this, bEnableVerboseLogging]()
        {
            FString HealthCheckUrl = ApiBaseUrl;
            HealthCheckUrl.RemoveFromEnd(TEXT("/"));
            HealthCheckUrl += TEXT("/health");

            if (bEnableVerboseLogging)
            {
                UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] Sending wake-up call to: %s"), *HealthCheckUrl);
            }

            TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
                FHttpModule::Get().CreateRequest();

            Request->SetURL(HealthCheckUrl);
            Request->SetVerb(TEXT("GET"));
            Request->SetTimeout(10.0f);

            Request->OnProcessRequestComplete().BindLambda(
                [this, bEnableVerboseLogging](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
                {
                    if (!Response.IsValid())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[UnifiedWebSocket] Wake-up failed: No response"));
                        return;
                    }

                    int32 Code = Response->GetResponseCode();

                    if (bWasSuccessful || Code == 404)
                    {
                        if (bEnableVerboseLogging)
                        {
                            UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] Cloud Run service warmed up successfully"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[UnifiedWebSocket] Wake-up call failed: %d"), Code);
                    }
                }
            );

            Request->ProcessRequest();
        },
        0.1f,
        false
    );
}


void UAiBridgeWebSocketSubsystem::PreFetchJwtToken()
{
    bJwtReady = false;

    AuthService->GetAuthToken(
        "UnifiedConnection",
        "player",
        "03BwqvuxqQaQ8m8i8r869nBfvf+nQj8uF8BTA9LgZR0=",
        [this](const FString& Token)
        {
            CachedToken = Token;
            bJwtReady = !Token.IsEmpty();

            UE_LOG(LogTemp, Log, TEXT("JWT ready. Token: %s"), *CachedToken);
        }
    );
}
