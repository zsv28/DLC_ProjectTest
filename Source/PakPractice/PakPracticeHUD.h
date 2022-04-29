// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PakPracticeHUD.generated.h"

UCLASS()
class APakPracticeHUD : public AHUD
{
	GENERATED_BODY()

public:
	APakPracticeHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

