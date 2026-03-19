// Fill out your copyright notice in the Description page of Project Settings.


#include "WebSocket/WebSocketConnection.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"


UWebSocketConnection::~UWebSocketConnection()
{
	if (WebSocket.IsValid()) {
		WebSocket->OnConnected().RemoveAll(this);
		WebSocket->OnConnectionError().RemoveAll(this);
		WebSocket->OnClosed().RemoveAll(this);
		WebSocket->OnMessage().RemoveAll(this);
		WebSocket->OnBinaryMessage().RemoveAll(this);
	}
}

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
	
	TWeakObjectPtr<UWebSocketConnection> WeakThis(this);
	
	WebSocket->OnConnected().AddLambda([WeakThis, Callback]()
	{
		if (!WeakThis.IsValid()) return;
		
		WeakThis->HandleConnected();
		Callback(true);
	});

	WebSocket->OnConnectionError().AddLambda(
		[WeakThis, Callback](const FString& Error)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleError(Error);
			Callback(false);
		}
	);
	
	WebSocket->OnClosed().AddLambda([WeakThis](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		if (!WeakThis.IsValid()) return;
		
		WeakThis->HandleClosed(StatusCode, Reason, bWasClean);
	});
	
	WebSocket->OnMessage().AddLambda(
		[WeakThis](const FString& Msg)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleOnMessage(Msg);
		}
	);

	WebSocket->OnBinaryMessage().AddLambda(
		[WeakThis](const void* Data, SIZE_T Size, bool isLast)
		{
			if (!WeakThis.IsValid()) return;
			
			WeakThis->HandleOnBinary(Data, Size, isLast);
		}
	);

	WebSocket->Connect();

	
}

void UWebSocketConnection::Disconnect()
{
	UE_LOG(LogTemp, Log, TEXT("UWebSocketConnection::Disconnect"));
	bAutoReconnect = false;
	bIsDisconnecting = true;

	if (WebSocket.IsValid())
	{
		WebSocket->Close();
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
	UE_LOG(LogTemp, Warning, TEXT("OnClosed"));
	
	if (bVerbose)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔌 Disconnected: %d"), StatusCode);
	}
	
	if (WebSocket.IsValid())
	{
		WebSocket->OnConnected().RemoveAll(this);
		WebSocket->OnConnectionError().RemoveAll(this);
		WebSocket->OnClosed().RemoveAll(this);
		WebSocket->OnMessage().RemoveAll(this);
		WebSocket->OnBinaryMessage().RemoveAll(this);
	}
	
	
	if (OnDisconnected) OnDisconnected();
	
	WebSocket = nullptr;
	
	if (bAutoReconnect)
	{
		//AttemptReconnect();
	}
}

void UWebSocketConnection::HandleError(const FString& Error)
{
	UE_LOG(LogTemp, Error, TEXT("WebSocket error: %s"), *Error);
	bIsConnecting = false;
	if (OnError) OnError(Error);
}

void UWebSocketConnection::HandleOnMessage(const FString& Msg)
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] Raw Message: %s"), *StaticClass()->GetName(), *Msg);
	if (OnTextMessage) OnTextMessage(Msg);
}

void UWebSocketConnection::HandleOnBinary(const void* Data, SIZE_T Size, bool isLast)
{
	TArray<uint8> Bytes;
	Bytes.Append((uint8*)Data, Size);

	if (OnBinaryMessage) OnBinaryMessage(Bytes);
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

	int32 TokenIndex = Result.Find(TEXT("token="), ESearchCase::IgnoreCase);
	if (TokenIndex == INDEX_NONE)
	{
		return Result;
	}

	int32 ValueStart = TokenIndex + 6; // length of "token="

	// Find next '&' AFTER token value
	int32 AmpIndex = Result.Find(TEXT("&"), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);

	if (AmpIndex != INDEX_NONE)
	{
		// token is in the middle
		return Result.Left(ValueStart) + TEXT("[REDACTED]") + Result.Mid(AmpIndex);
	}
	else
	{
		// token is last parameter
		return Result.Left(ValueStart) + TEXT("[REDACTED]");
	}
}