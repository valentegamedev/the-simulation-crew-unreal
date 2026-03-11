// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RequestOrchestrator.generated.h"

UCLASS()
class AIBRIDGE_API ARequestOrchestrator : public AActor
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Default, meta = (AllowPrivateAccess = "true"))
	FString ApiBaseUrl;
	
public:	
	// Sets default values for this actor's properties
	ARequestOrchestrator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(TScriptInterface<IApiKeyProvider> ApiKeyProvider);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();
};
