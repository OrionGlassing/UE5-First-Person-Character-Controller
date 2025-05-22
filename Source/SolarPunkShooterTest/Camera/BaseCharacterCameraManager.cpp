// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacterCameraManager.h"

#include "INetworkMessagingExtension.h"
#include "Camera/CameraComponent.h"
#include "Chaos/ArrayND.h"
#include "Components/CapsuleComponent.h"
#include "SolarPunkShooterTest/Character/BaseCharacter.h"
#include "SolarPunkShooterTest/Movement/TestMovementComponent.h"

ABaseCharacterCameraManager::ABaseCharacterCameraManager()
{
	
}

void ABaseCharacterCameraManager::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = MinFOV;
	PreviousTargetFOV = MinFOV;
	PreviousTargetSidewaysTilt = 0.f;
	TargetWallRunRotation = 0.f;

}

void ABaseCharacterCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	
	
	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetOwningPlayerController()->GetPawn()))
	{
		UTestMovementComponent* TMC = BaseCharacter->GetBaseCharacterMovement();

		/* TO DO LIST:
		 *	- Implement Mantle Mechanics
		 *		- Move and or Rotate Camera
		 *		- Maybe play a screen shake
		 *	- Add Screen Shakes
		 *		- Walking / Running Shakes
		 *		- Start Grapple Shake
		 *		- Etc.
		*/

		//
		// Adjust FOV Based on Horizontal Player Velocity
		//
		
		UE::Math::TVector2 InputRange = {0.f, TMC->Tac_MaxSpeed * 1.25f};
        UE::Math::TVector2 OutputRange = {MinFOV, MaxFOV};
        float InputValue = TMC->Velocity.Size2D();
        float TargetFOV = FMath::GetMappedRangeValueClamped(InputRange, OutputRange, InputValue);

		if(TargetFOV >= PreviousTargetFOV)
		{
			TargetFOV = FMath::FInterpTo(PreviousTargetFOV, TargetFOV, DeltaTime, IncreaseFOVTime);
		}
		else
		{
			TargetFOV = FMath::FInterpTo(PreviousTargetFOV, TargetFOV, DeltaTime, ReduceFOVTime);
		}
		
		OutVT.POV.FOV = TargetFOV;
		PreviousTargetFOV = TargetFOV;

		//
		// Momentum Based Tilt
		//

		//Sideways Tilt
		float TargetSidewaysTilt;
		if (BaseCharacter->IsStrafing)
		{
			if (!TMC->IsFalling())
			{
				if (BaseCharacter->bStrafeRight)
	            {
            		TargetSidewaysTilt = FVector::DotProduct(TMC->Velocity.GetSafeNormal2D(), BaseCharacter->GetActorRightVector().GetSafeNormal2D());
            		TargetSidewaysTilt = FMath::Clamp(TargetSidewaysTilt, 0.f, 1.f);
            		TargetSidewaysTilt *= SidewaysTiltMax;
	            }
	            else
	            {
            		TargetSidewaysTilt = FVector::DotProduct(TMC->Velocity.GetSafeNormal2D(), -BaseCharacter->GetActorRightVector().GetSafeNormal2D());
            		TargetSidewaysTilt = FMath::Clamp(TargetSidewaysTilt, 0.f, 1.f);
            		TargetSidewaysTilt *= -SidewaysTiltMax;
	            }
	            TargetSidewaysTilt = FMath::FInterpTo(PreviousTargetSidewaysTilt, TargetSidewaysTilt, DeltaTime, MomentumTiltTime);
			}
			else
			{
				TargetSidewaysTilt = FMath::FInterpTo(PreviousTargetSidewaysTilt, 0.f, DeltaTime, MomentumTiltTime);
			}
		}
		else
		{
			TargetSidewaysTilt = FMath::FInterpTo(PreviousTargetSidewaysTilt, 0.f, DeltaTime, MomentumTiltTime);
		}
		OutVT.POV.Rotation.Roll += TargetSidewaysTilt;
		PreviousTargetSidewaysTilt = TargetSidewaysTilt;

		//Forward-Backward Tilt
		float TargetForBackTilt;
		if (BaseCharacter->IsMovingForward && !TMC->IsFalling())
		{
			float DotMulti = FVector::DotProduct(TMC->Velocity.GetSafeNormal2D(), BaseCharacter->GetActorForwardVector().GetSafeNormal2D());
			TargetForBackTilt = (FMath::Clamp(TMC->Velocity.Size2D() / TMC->Tac_MaxSpeed, 0.f, 1.f)) * ForBackTiltMax * DotMulti;
		}
		else if (BaseCharacter->IsMovingBack && !TMC->IsFalling())
		{
			float DotMulti = FVector::DotProduct(TMC->Velocity.GetSafeNormal2D(), -BaseCharacter->GetActorForwardVector().GetSafeNormal2D());
			TargetForBackTilt = (FMath::Clamp(TMC->Velocity.Size2D() / TMC->Walk_MaxSpeed, 0.f, 1.f)) * -ForBackTiltMax * DotMulti;
		}
		else
		{
			TargetForBackTilt = 0.f;
		}
		TargetForBackTilt = FMath::FInterpTo(PreviousTargetForBackTilt, TargetForBackTilt, DeltaTime, MomentumTiltTime);
		OutVT.POV.Rotation.Pitch += TargetForBackTilt;
		PreviousTargetForBackTilt = TargetForBackTilt;

		//Falling Tilt
		float TargetFallingTilt;
		if (TMC->IsFalling())
		{
			TargetFallingTilt = FMath::Clamp(FMath::Abs(TMC->Velocity.Z) / TMC->JumpZVelocity, 0.f, 1.f) * FallingTiltMax;
			if (TMC->Velocity.Z < 0.f)
			{
				TargetFallingTilt = -TargetFallingTilt;
			}
		}
		else
		{
			TargetFallingTilt = 0.f;
		}
		TargetFallingTilt = FMath::FInterpTo(PreviousTargetFallingTilt, TargetFallingTilt, DeltaTime, MomentumTiltTime);
		OutVT.POV.Rotation.Pitch += TargetFallingTilt;
		PreviousTargetFallingTilt = TargetFallingTilt;
		
		
		//
		// Crouch Position Adjustment
		//

		float TargetCapsuleOffset;

		if (TMC->IsMovingOnGround())
		{
			TargetCapsuleOffset = TMC->CapHH() -
				BaseCharacter->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
		else
		{
			TargetCapsuleOffset = 0.f;
		}

		TargetCapsuleOffset = FMath::FInterpTo(PreviousCapsuleTarget, TargetCapsuleOffset, DeltaTime, CapsuleAdjustTime);

		
		FVector VectorCapsuleOffset = FVector(
			0,
			0,
			TargetCapsuleOffset);

		OutVT.POV.Location += VectorCapsuleOffset;
		
		PreviousCapsuleTarget = TargetCapsuleOffset;
		
		//
		// Wall Run Rotation Adjustment
		//
		float RollOffset;
		if (TMC->IsWallRunning)
		{
			if (TMC->WallRunIsRight)
			{
				TargetWallRunRotation = WallRunRotation;
			}
			else
			{
				TargetWallRunRotation = -WallRunRotation;
			}
			WallRunBlendTime = FMath::Clamp(WallRunBlendTime + DeltaTime, 0.f, WallRunRotateBlendDuration);

			RollOffset = FMath::InterpEaseInOut(0.f, TargetWallRunRotation,
			FMath::Clamp(WallRunBlendTime / WallRunRotateBlendDuration, 0.f, 1.f), WallRunRotateExponent);
		}
		else
		{
			WallRunBlendTime = FMath::Clamp(WallRunBlendTime - DeltaTime, 0.f, WallRunRotateBlendDuration);

			RollOffset = FMath::InterpEaseIn(0.f, TargetWallRunRotation,
			FMath::Clamp(WallRunBlendTime / WallRunRotateBlendDuration, 0.f, 1.f), WallRunRotateExponent);
		}
		
		OutVT.POV.Rotation.Roll += RollOffset;

		//
		//update publicly accessible updated viewtarget info (maybe replace)
		//
		//ViewTargetLocation = OutVT.POV.Location;
		//ViewTargetRotation = OutVT.POV.Rotation;
	}
}