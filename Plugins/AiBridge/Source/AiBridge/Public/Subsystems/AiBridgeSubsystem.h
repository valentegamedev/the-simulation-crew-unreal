// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AiBridgeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisconnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBinaryMessage, const TArray<uint8>&, Data);

class UJwtAuthenticationService;
class UWebSocketConnection;
/**
 * 
 */
UCLASS()
class AIBRIDGE_API UAiBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	FString ApiKeyProvider;
	FString ApiBaseUrl;
	
	UPROPERTY()
	UJwtAuthenticationService* AuthService;
	
	UPROPERTY()
	UWebSocketConnection* WebSocket;
	
	bool bJwtReady;
	FString CachedToken;
	bool bIsConnecting = false;
public:
	// Events
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnConnected OnConnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnDisconnected OnDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnTextMessage OnTextMessage;

	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnBinaryMessage OnBinaryMessage;
	
private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void SendWakeUpCallAsync() const;
	
public:
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(FString ApiKeyProvider, FString ApiBaseUrl);
	
	void EnsureConnection(TFunction<void(bool)> Callback);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();
};
