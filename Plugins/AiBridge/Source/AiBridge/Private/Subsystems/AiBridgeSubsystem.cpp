// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeSubsystem.h"
#include "Authentication/JwtAuthenticationService.h"

void UAiBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AuthService = NewObject<UJwtAuthenticationService>(this);
	
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
	
	if (!AuthService)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] JwtAuthenticationService not assigned."), *StaticClass()->GetName());
		return;
	}
	AuthService->Initialize(ApiBaseUrl);
	UE_LOG(LogTemp, Warning, TEXT("[%s] JwtAuthenticationService Initialized on ApiBaseUrl = %s"), *StaticClass()->GetName(), *ApiBaseUrl);
	
	AuthService->GetAuthToken(
		"UnifiedConnection",
		"player",
		ApiKeyProvider,
		[this](const FString& Token)
		{
			CachedToken = Token;
			bJwtReady = !Token.IsEmpty();

			UE_LOG(LogTemp, Warning, TEXT("[%s] JWT ready. Token: %s"), *StaticClass()->GetName(), *CachedToken);
		}
	);
}

void UAiBridgeSubsystem::Disconnect()
{

}