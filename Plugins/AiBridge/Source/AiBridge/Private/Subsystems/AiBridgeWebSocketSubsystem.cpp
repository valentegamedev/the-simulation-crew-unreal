// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AiBridgeWebSocketSubsystem.h"

#include "WebSocketsModule.h"
#include "Authentication/JwtAuthenticationService.h"
#include "WebSocket/WebSocketConnection.h"

void UAiBridgeWebSocketSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    AuthService = NewObject<UJwtAuthenticationService>(this);
    AuthService->Initialize(ApiBaseUrl);
    
    InitializeConnectionSequence();
    
    UE_LOG(LogTemp, Log, TEXT("UAiBridgeWebSocketSubsystem Initialized"));
}

void UAiBridgeWebSocketSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("UAiBridgeWebSocketSubsystem Deinitialized"));
    Disconnect();
    Super::Deinitialize();
}

void UAiBridgeWebSocketSubsystem::Connect()
{
    EnsureConnection([this](bool Success) { 
        
        UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] Connected to server %d"), Success);
        
    } );
    /*
    if (WebSocket.IsValid())
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
    */
}

void UAiBridgeWebSocketSubsystem::Disconnect()
{
    if (WebSocket!= nullptr)
    {
        WebSocket->Disconnect();
    }
}

void UAiBridgeWebSocketSubsystem::SendText(const FString& Message)
{
    if (WebSocket!= nullptr && WebSocket->IsConnected())
    {
        WebSocket->SendText(Message);
    }
}

void UAiBridgeWebSocketSubsystem::SendBinary(const TArray<uint8>& Data)
{
    if (WebSocket!= nullptr && WebSocket->IsConnected())
    {
        WebSocket->SendBinary(Data);
    }
}

bool UAiBridgeWebSocketSubsystem::IsConnected() const
{
    return WebSocket!= nullptr && WebSocket->IsConnected();
}

void UAiBridgeWebSocketSubsystem::EnsureConnection(TFunction<void(bool)> Callback)
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
        TEXT("03BwqvuxqQaQ8m8i8r869nBfvf+nQj8uF8BTA9LgZR0="),
        [this, Callback, StartTime](const FString& JwtToken)
        {
            double JwtTime = (FPlatformTime::Seconds() - StartTime) * 1000.0;

            bool bEnableVerboseLogging = true;
            if (bEnableVerboseLogging)
            {
                UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket New] JWT took %.0f ms"), JwtTime);
            }

            if (JwtToken.IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to get JWT"));
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
                TEXT("/api/websocket") // your endpoint
            );

            FString FullUrl = FString::Printf(TEXT("%s?token=%s"),
                *BaseUrl,
                *FGenericPlatformHttp::UrlEncode(JwtToken)
            );
            
            UE_LOG(LogTemp, Log, TEXT("[On WebSocket] URL: %s"), *FullUrl);
            
            
            bEnableVerboseLogging = true;
            
            // Create WS
            WebSocket = NewObject<UWebSocketConnection>(this);

            
            
            // Bind events
            WebSocket->OnTextMessage = [this](const FString& Msg)
            {
                UE_LOG(LogTemp, Log, TEXT("[MEssage] %s"), *Msg);
            };

            WebSocket->OnBinaryMessage = [this](const TArray<uint8>& Data)
            {
                FString Result = FString(Data.Num(), UTF8_TO_TCHAR(reinterpret_cast<const char*>(Data.GetData())));
                UE_LOG(LogTemp, Log, TEXT("[On Binary] %s"), *Result);
            };

            WebSocket->OnDisconnected = [this]()
            {
                UE_LOG(LogTemp, Log, TEXT("[disconnect]"));
            };

            double WsStart = FPlatformTime::Seconds();

            WebSocket->Connect(
                FullUrl,
                TEXT("UnifiedConnection"),
                JwtToken,
                [this, Callback, StartTime, WsStart, FullUrl, &bEnableVerboseLogging](bool bConnected)
                {
                    double WsTime = (FPlatformTime::Seconds() - WsStart) * 1000.0;

                    if (bEnableVerboseLogging)
                    {
                        UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] WS took %.0f ms"), WsTime);
                    }

                    if (bConnected)
                    {

                        double Total = (FPlatformTime::Seconds() - StartTime) * 1000.0;

                        UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] Connected (%.0f ms total)"), Total);

                        bIsConnecting = false;
                        Callback(true);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("WebSocket failed"));

                        bIsConnecting = false;

                        bIsConnecting = false;
                        Callback(false);
                    }
                }
            );
            
        }
    );
    
}

void UAiBridgeWebSocketSubsystem::SendSomethingCrazy()
{
    //F2FF1DD549380EB9EF7DAA80CA9AC7FF
    FString JsonString = TEXT(R"(
        {
          "typ: 1.0,
            "ttsLanguageCode": "en",

            "responseFormat": "json_object",
            "location": "europe-west4",

            "contextCacheName": "projects/my-project/locations/europe-west4/cachedContents/abc123"
          }
        }
    )");
    FString GuidString = FGuid::NewGuid().ToString();
    
    UE_LOG(LogTemp, Warning, TEXT("%s"), *GuidString);
    WebSocket->SendText(JsonString);
}

void UAiBridgeWebSocketSubsystem::SendSomething()
{
    FString JsonString = TEXT(R"(
        {
          "type": "textinput",
          "text": "Hello, how are you today?",
          "requestId": "F2FF1DD549380EB9EF7DAA80CA9AC7FF",
          "timestamp":1771596968241,
          "isNpcInitiated": false,
          "context": {
            "systemPrompt": "You are a professional customer service agent for XRLab.\n\nCOMPANY INFORMATION:\nSaxion XRLab is a Mixed Reality lab which focus on innovation using VR and AR solutions.\n\nPRODUCT KNOWLEDGE:\nWe offer development services for any kind of media which needs VR or AR. Including development using Unity and Unreal.\n\nCUSTOMER CONTEXT:\n\"No customer context available.\"\n\nSERVICE GUIDELINES:\n1. Greet customers warmly and professionally\n2. Listen actively to understand the issue\n3. Provide accurate information from the knowledge base\n4. If you don't know something, say so and offer to escalate\n5. Always confirm the customer's issue is resolved before ending\n6. Keep responses concise but complete\n\nESCALATION TRIGGERS:\n- Technical issues beyond basic troubleshooting\n- Billing disputes over ${serviceConfig.escalationThreshold}\n- Complaints about employee conduct\n- Legal or compliance questions\n\nWhen escalating, explain why and what will happen next.",
            "messages": [{
                "role": "user",
                "content": "Hi there!"
              },
              {
                "role": "assistant",
                "content": "Hello traveler! What brings you here?"
              }],
            "voiceId": "EXAVITQu4vr4xnSDxMaL",
            "llmModel": "gpt-4o-mini",
            "llmProvider": "openai",
            "temperature": 0.7,
            "maxTokens": 500,
            "language": "en-US",
            "ttsStreamingMode": "batch",
            "ttsModel": "eleven_turbo_v2_5",
            "sttProvider": "google",

            "voiceStability": 0.5,
            "voiceSimilarityBoost": 0.75,
            "voiceStyle": 0.6,
            "voiceUseSpeakerBoost": true,
            "voiceSpeed": 1.0,
            "ttsLanguageCode": "en",

            "responseFormat": "json_object",
            "location": "europe-west4",

            "contextCacheName": "projects/my-project/locations/europe-west4/cachedContents/abc123"
          }
        }
    )");
    WebSocket->SendText(JsonString);
}

void UAiBridgeWebSocketSubsystem::InitializeConnectionSequence()
{
    if (sendWakeUpCall)
    {
        SendWakeUpCallAsync();
    }
    PreFetchJwtToken();
}

void UAiBridgeWebSocketSubsystem::SendWakeUpCallAsync() const
{
    bool bEnableVerboseLogging = true;
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UnifiedWebSocket] No World available"));
        return;
    }
    
    FTimerHandle TimerHandle;

    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        [this, bEnableVerboseLogging]()
        {
            FString HealthCheckUrl = ApiBaseUrl;
            HealthCheckUrl.RemoveFromEnd(TEXT("/"));
            HealthCheckUrl += TEXT("/health");

            if (bEnableVerboseLogging)
            {
                UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] Sending wake-up call to: %s"), *HealthCheckUrl);
            }

            TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
                FHttpModule::Get().CreateRequest();

            Request->SetURL(HealthCheckUrl);
            Request->SetVerb(TEXT("GET"));
            Request->SetTimeout(10.0f);

            Request->OnProcessRequestComplete().BindLambda(
                [this, bEnableVerboseLogging](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
                {
                    if (!Response.IsValid())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[UnifiedWebSocket] Wake-up failed: No response"));
                        return;
                    }

                    int32 Code = Response->GetResponseCode();

                    if (bWasSuccessful || Code == 404)
                    {
                        if (bEnableVerboseLogging)
                        {
                            UE_LOG(LogTemp, Log, TEXT("[UnifiedWebSocket] Cloud Run service warmed up successfully"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[UnifiedWebSocket] Wake-up call failed: %d"), Code);
                    }
                }
            );

            Request->ProcessRequest();
        },
        0.1f,
        false
    );
}


void UAiBridgeWebSocketSubsystem::PreFetchJwtToken()
{
    bJwtReady = false;

    AuthService->GetAuthToken(
        "UnifiedConnection",
        "player",
        "03BwqvuxqQaQ8m8i8r869nBfvf+nQj8uF8BTA9LgZR0=",
        [this](const FString& Token)
        {
            CachedToken = Token;
            bJwtReady = !Token.IsEmpty();

            UE_LOG(LogTemp, Log, TEXT("JWT ready. Token: %s"), *CachedToken);
        }
    );
}
