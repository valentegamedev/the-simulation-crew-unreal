// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeSubsystem.h"
#include "Auth/ApiKeyProvider.h"

void UAiBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] AiBridge Initialized."), *StaticClass()->GetName());
}

void UAiBridgeSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] AiBridge Deinitialized."), *StaticClass()->GetName());
	Super::Deinitialize();
}

void UAiBridgeSubsystem::Connect(FString pApiKeyProvider, FString pApiBaseUrl)
{
	ApiKeyProvider = pApiKeyProvider;
	ApiBaseUrl = pApiBaseUrl;
	if (ApiKeyProvider.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ApiKey is invalid."), *StaticClass()->GetName());
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[%s] ApiKey is valid."), *StaticClass()->GetName());
	
}

void UAiBridgeSubsystem::Disconnect()
{

}