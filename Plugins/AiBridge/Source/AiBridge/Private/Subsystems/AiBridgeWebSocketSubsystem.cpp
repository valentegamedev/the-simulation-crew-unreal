// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeWebSocketSubsystem.h"

#include "WebSocketsModule.h"

void UAiBridgeWebSocketSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
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