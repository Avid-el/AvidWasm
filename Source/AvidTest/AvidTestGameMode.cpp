// Copyright Epic Games, Inc. All Rights Reserved.

#include "AvidTestGameMode.h"
#include "AvidTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAvidTestGameMode::AAvidTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
