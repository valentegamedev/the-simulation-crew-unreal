// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheSimulationCrewGameMode.h"
#include "TheSimulationCrewCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATheSimulationCrewGameMode::ATheSimulationCrewGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
