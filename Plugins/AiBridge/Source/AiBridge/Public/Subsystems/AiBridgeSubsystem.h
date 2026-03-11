// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AiBridgeSubsystem.generated.h"

class  IApiKeyProvider;

/**
 * 
 */
UCLASS()
class AIBRIDGE_API UAiBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	FString ApiKeyProvider;
	FString ApiBaseUrl;
	
private:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(FString ApiKeyProvider, FString ApiBaseUrl);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();
};
