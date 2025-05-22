// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "BaseCharacterCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class SOLARPUNKSHOOTERTEST_API ABaseCharacterCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	// functions
	ABaseCharacterCameraManager();
	virtual void BeginPlay() override;
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
	
	// public accessible info
	// MAYBE NOT NEEDED - IN .H FILE THERE IS "TransformComponent" CLAIMS TO BE USED TO ATTACH THINGS TO CAMERA
	FVector ViewTargetLocation;
	FRotator ViewTargetRotation;

protected: 
	// FOV Change
    UPROPERTY(EditDefaultsOnly) float MinFOV;
    UPROPERTY(EditDefaultsOnly) float MaxFOV;
    UPROPERTY(EditDefaultsOnly) float IncreaseFOVTime;
    UPROPERTY(EditDefaultsOnly) float ReduceFOVTime;
	float PreviousTargetFOV;

	//Momentum Tilt
	UPROPERTY(EditDefaultsOnly) float MomentumTiltTime;
	//Sideways Tilt
	UPROPERTY(EditDefaultsOnly) float SidewaysTiltMax;
	float PreviousTargetSidewaysTilt;
	//Forward-Backward Tilt
	UPROPERTY(EditDefaultsOnly) float ForBackTiltMax;
	float PreviousTargetForBackTilt;
	//Falling Tilt
	UPROPERTY(EditDefaultsOnly) float FallingTiltMax;
	float PreviousTargetFallingTilt;
	
	//Slide
	UPROPERTY(EditDefaultsOnly) float CapsuleAdjustTime;
	float PreviousCapsuleTarget;

	
	//Wallrun
	UPROPERTY(EditDefaultsOnly) float WallRunRotation;
	float TargetWallRunRotation;
	UPROPERTY(EditDefaultsOnly) float WallRunRotateBlendDuration;
	float WallRunBlendTime;
	UPROPERTY(EditDefaultsOnly) float WallRunRotateExponent;


};
