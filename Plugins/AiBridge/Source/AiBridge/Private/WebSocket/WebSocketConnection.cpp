// Fill out your copyright notice in the Description page of Project Settings.


#include "WebSocket/WebSocketConnection.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"



bool UWebSocketConnection::IsConnected() const
{
	return WebSocket.IsValid() && WebSocket->IsConnected();
}

void UWebSocketConnection::Connect(const FString& Url, const FString& ConnectionId, const FString& InToken, TFunction<void(bool)> Callback)
{
	if (IsConnected() || bIsConnecting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already connected or connecting"));
		Callback(false);
		return;
	}

	bIsConnecting = true;
	JwtToken = InToken;
	LastUrl = Url;

	FString SafeUrl = SanitizeUrl(Url);

	UE_LOG(LogTemp, Log, TEXT("🔌 Connecting to %s"), *SafeUrl);

	WebSocket = FWebSocketsModule::Get().CreateWebSocket(Url);

	//WebSocket->OnConnected().AddRaw(this, &UWebSocketConnection::HandleConnected);
	WebSocket->OnConnected().AddLambda([this]()
	{
		HandleConnected();
	});

	WebSocket->OnConnectionError().AddLambda(
		[this, Callback](const FString& Error)
		{
			HandleError(Error);
			bIsConnecting = false;
			Callback(false);
		}
	);

	//WebSocket->OnClosed().AddRaw(this, &UWebSocketConnection::HandleClosed);
	WebSocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnClosed"));
		//HandleClosed(StatusCode, Reason, bWasClean);
	});
	
	WebSocket->OnMessage().AddLambda(
		[this](const FString& Msg)
		{
			if (OnTextMessage) OnTextMessage(Msg);
		}
	);

	WebSocket->OnBinaryMessage().AddLambda(
		[this](const void* Data, SIZE_T Size, bool isLast)
		{
			TArray<uint8> Bytes;
			Bytes.Append((uint8*)Data, Size);

			if (OnBinaryMessage) OnBinaryMessage(Bytes);
		}
	);

	WebSocket->Connect();

	// Simulate "await connection with timeout"
	FTimerHandle Timer;
	GWorld->GetTimerManager().SetTimer(
		Timer,
		[this, Callback]()
		{
			bool bSuccess = IsConnected();
			bIsConnecting = false;
			Callback(bSuccess);
		},
		10.0f,
		false
	);
}

void UWebSocketConnection::Disconnect()
{
	bAutoReconnect = false;
	bIsDisconnecting = true;

	if (WebSocket.IsValid())
	{
		WebSocket->Close();
		WebSocket = nullptr;
	}
}

void UWebSocketConnection::HandleConnected()
{
	bIsConnecting = false;
	ReconnectAttempts = 0;
	CurrentReconnectDelay = ReconnectBaseDelay;

	if (bVerbose)
	{
		UE_LOG(LogTemp, Log, TEXT("✅ Connected"));
	}

	if (OnConnected) OnConnected();
}

void UWebSocketConnection::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	if (bVerbose)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔌 Disconnected: %d"), StatusCode);
	}

	if (OnDisconnected) OnDisconnected();

	if (bAutoReconnect)
	{
		//AttemptReconnect();
	}
}

void UWebSocketConnection::HandleError(const FString& Error)
{
	UE_LOG(LogTemp, Error, TEXT("WebSocket error: %s"), *Error);

	if (OnError) OnError(Error);
}

void UWebSocketConnection::AttemptReconnect()
{
	if (bIsReconnecting || ReconnectAttempts >= MaxReconnectAttempts)
		return;

	bIsReconnecting = true;
	ReconnectAttempts++;

	float Delay = CurrentReconnectDelay;

	UE_LOG(LogTemp, Warning, TEXT("Reconnect attempt %d in %.1fs"), ReconnectAttempts, Delay);

	FTimerHandle Timer;

	GWorld->GetTimerManager().SetTimer(
		Timer,
		[this]()
		{
			bIsReconnecting = false;
			/*
			Connect(LastUrl, JwtToken,
				[](bool bSuccess)
				{
					// handled internally
				});
			*/
			CurrentReconnectDelay = FMath::Min(CurrentReconnectDelay * 2.f, ReconnectMaxDelay);
		},
		Delay,
		false
	);
}

void UWebSocketConnection::SendText(const FString& Message)
{
	if (!IsConnected()) return;

	WebSocket->Send(Message);
}

void UWebSocketConnection::SendBinary(const TArray<uint8>& Data)
{
	if (!IsConnected()) return;

	WebSocket->Send(Data.GetData(), Data.Num(), true);
}

FString UWebSocketConnection::SanitizeUrl(const FString& Url)
{
	FString Result = Url;

	Result = Result.Replace(TEXT("token="), TEXT("token=[REDACTED]"));

	return Result;
}