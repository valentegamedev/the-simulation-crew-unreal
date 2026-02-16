// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "AiBridgeWebSocketSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWebSocketConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWebSocketDisconnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebSocketTextMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebSocketBinaryMessage, const TArray<uint8>&, Data);


/**
 * 
 */
UCLASS()
class AIBRIDGE_API UAiBridgeWebSocketSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

	// Connection
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();

	// Sending
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendText(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendBinary(const TArray<uint8>& Data);

	UFUNCTION(BlueprintPure, Category = "WebSocket")
	bool IsConnected() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketConnected OnConnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketDisconnected OnDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketTextMessage OnTextMessage;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnWebSocketBinaryMessage OnBinaryMessage;

private:
	TSharedPtr<IWebSocket> Socket;
	
};
