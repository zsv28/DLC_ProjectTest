// Copyright Epic Games, Inc. All Rights Reserved.

#include "PakPracticeGameMode.h"
#include "PakPracticeHUD.h"
#include "PakPracticeCharacter.h"
#include "UObject/ConstructorHelpers.h"

APakPracticeGameMode::APakPracticeGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = APakPracticeHUD::StaticClass();
}
