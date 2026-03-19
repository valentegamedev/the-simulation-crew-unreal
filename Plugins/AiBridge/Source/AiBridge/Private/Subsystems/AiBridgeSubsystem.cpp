// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeSubsystem.h"
#include "Authentication/JwtAuthenticationService.h"
#include "WebSocket/WebSocketConnection.h"

void UAiBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (!IsValid(AuthService)) { 
		AuthService = NewObject<UJwtAuthenticationService>(this);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] AiBridge Initialized."), *StaticClass()->GetName());
}

void UAiBridgeSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] AiBridge Deinitialized."), *StaticClass()->GetName());
	Super::Deinitialize();
}

void UAiBridgeSubsystem::SendWakeUpCallAsync() const
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] No World available."), *StaticClass()->GetName());
		return;
	}
    
	FTimerHandle TimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		[this]()
		{
			FString HealthCheckUrl = ApiBaseUrl;
			HealthCheckUrl.RemoveFromEnd(TEXT("/"));
			HealthCheckUrl += TEXT("/health");
			
			UE_LOG(LogTemp, Warning, TEXT("[%s] Sending wake-up call to: %s"), *StaticClass()->GetName(), *HealthCheckUrl);

			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
				FHttpModule::Get().CreateRequest();

			Request->SetURL(HealthCheckUrl);
			Request->SetVerb(TEXT("GET"));
			Request->SetTimeout(10.0f);

			Request->OnProcessRequestComplete().BindLambda(
				[this](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
				{
					if (!Response.IsValid())
					{
						UE_LOG(LogTemp, Warning, TEXT("[%s] Wake-up failed: No response."), *StaticClass()->GetName());
						return;
					}

					int32 Code = Response->GetResponseCode();

					if (bWasSuccessful || Code == 404)
					{
						UE_LOG(LogTemp, Warning, TEXT("[%s] Cloud Run service warmed up successfully."), *StaticClass()->GetName());
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[%s] Wake-up call failed."), *StaticClass()->GetName());
					}
				}
			);

			Request->ProcessRequest();
		},
		0.1f,
		false
	);
}

void UAiBridgeSubsystem::Connect(FString pApiKeyProvider, FString pApiBaseUrl)
{
	ApiKeyProvider = pApiKeyProvider;
	ApiBaseUrl = pApiBaseUrl;
	
	SendWakeUpCallAsync();
	
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
		[this](const FString& JwtToken)
		{
			CachedToken = JwtToken;
			bJwtReady = !JwtToken.IsEmpty();

			UE_LOG(LogTemp, Warning, TEXT("[%s] JWT ready. Token: %s"), *StaticClass()->GetName(), *CachedToken);
			
			EnsureConnection([this](bool Success) { 
				UE_LOG(LogTemp, Warning, TEXT("[%s] Connected to server: %d"), *StaticClass()->GetName(), Success);
			});
		}
	);
	
}

void UAiBridgeSubsystem::EnsureConnection(TFunction<void(bool)> Callback)
{
	// 1. Already connected
    if (WebSocket!= nullptr && WebSocket->IsConnected())
    {
        Callback(true);
        return;
    }
	
    // 3. Start connection
    bIsConnecting = true;

    double StartTime = FPlatformTime::Seconds();
    
    AuthService->GetAuthToken(
        TEXT("UnifiedConnection"),
        TEXT("player"),
        ApiKeyProvider,
        [this, Callback, StartTime](const FString& JwtToken)
        {
            double JwtTime = (FPlatformTime::Seconds() - StartTime) * 1000.0;
        	
        	UE_LOG(LogTemp, Warning, TEXT("[%s] JWT took %.0f ms"), *StaticClass()->GetName(), JwtTime);

            if (JwtToken.IsEmpty())
            {
            	UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to get JWT."), *StaticClass()->GetName());
                bIsConnecting = false;
                Callback(false);
                return;
            }
        	
            // Build URL
            FString WsScheme = ApiBaseUrl.StartsWith(TEXT("https")) ? TEXT("wss") : TEXT("ws");

            FString WsBase = ApiBaseUrl;
            WsBase.ReplaceInline(TEXT("http://"), TEXT(""));
            WsBase.ReplaceInline(TEXT("https://"), TEXT(""));

            FString BaseUrl = FString::Printf(TEXT("%s://%s%s"),
                *WsScheme,
                *WsBase.TrimEnd(),
                TEXT("/api/websocket")
            );

            FString FullUrl = FString::Printf(TEXT("%s?token=%s"),
                *BaseUrl,
                *FGenericPlatformHttp::UrlEncode(JwtToken)
            );
        	
            // Create WS
        	if (!IsValid(WebSocket))
        	{
        		WebSocket = NewObject<UWebSocketConnection>(this);
        	}
        	
        	double WsStart = FPlatformTime::Seconds();
        	
        	WebSocket->OnDisconnected = [this]()
			{
				UE_LOG(LogTemp, Log, TEXT("[disconnect]"));
			};
        	
            WebSocket->Connect(
                FullUrl,
                TEXT("UnifiedConnection"),
                JwtToken,
                [this, Callback, StartTime, WsStart, FullUrl](bool bConnected)
                {
                    double WsTime = (FPlatformTime::Seconds() - WsStart) * 1000.0;

                	UE_LOG(LogTemp, Warning, TEXT("[%s] WS took %.0f ms"), *StaticClass()->GetName(), WsTime);
                	
                    if (bConnected)
                    {
                        double Total = (FPlatformTime::Seconds() - StartTime) * 1000.0;

                    	UE_LOG(LogTemp, Warning, TEXT("[%s] Connected (%.0f ms total)"), *StaticClass()->GetName(), Total);

                        bIsConnecting = false;
                    	OnConnected.Broadcast();
                        Callback(true);
                    }
                    else
                    {
                    	UE_LOG(LogTemp, Error, TEXT("[%s] WebSocket failed."), *StaticClass()->GetName());

                        bIsConnecting = false;
                        Callback(false);
                    }
                }
            );
        }
    );
}

void UAiBridgeSubsystem::Disconnect()
{

}
