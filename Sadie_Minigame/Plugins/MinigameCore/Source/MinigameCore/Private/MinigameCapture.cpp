/**************************************************************************************************
 * MinigameCapture.cpp
 * LIMBITLESS SOLUTIONS INC.                                                              2/21/2025
 * https://limbitless-solutions.org/                                                         
 * 
 * Author:  Donny Akers
 * 
 * Copyright (C) 2025 Limbitless Solutions Inc.
 * Copyright (C) 2025 University of Central Florida
 * 
 * This work is licensed under the:
 * Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/ 
 * or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 */

#include "MinigameCapture.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Math/UnrealMathUtility.h"


AMinigameCapture::AMinigameCapture()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(FName("Root")));
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(FName(TEXT("Scene Capture")));
	SceneCapture->SetupAttachment(GetRootComponent());

	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bCaptureEveryFrame = false;
}

void AMinigameCapture::ApplyCameraShake(float Intensity, float Duration)
{
	CameraShakeIntensity += Intensity;
	
	FTimerHandle RemoveShakeHandle{};
	GetWorldTimerManager().SetTimer(RemoveShakeHandle, [=]() { CameraShakeIntensity -= Intensity; }, Duration, false);
}


void AMinigameCapture::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	float Time = GetGameTimeSinceCreation();

	FVector ShakeOffset
	{
		FMath::GetMappedRangeValueClamped(FVector2f{ -1.f, 1.f }, FVector2f{ -CameraShakeIntensity, CameraShakeIntensity }, FMath::PerlinNoise1D(Time)),
		FMath::GetMappedRangeValueClamped(FVector2f{ -1.f, 1.f }, FVector2f{ -CameraShakeIntensity, CameraShakeIntensity }, FMath::PerlinNoise1D(Time * 6553)),
		FMath::GetMappedRangeValueClamped(FVector2f{ -1.f, 1.f }, FVector2f{ -CameraShakeIntensity, CameraShakeIntensity }, FMath::PerlinNoise1D(Time * 1337)),
	};

	SceneCapture->SetRelativeLocation(ShakeOffset);
}

void AMinigameCapture::EnableCamera(bool bIsEnabled)
{
	SceneCapture->bCaptureEveryFrame = bIsEnabled;
}

