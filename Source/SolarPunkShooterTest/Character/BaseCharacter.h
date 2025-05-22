// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

UCLASS()
class SOLARPUNKSHOOTERTEST_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement)
	class UTestMovementComponent* TestMovementComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Abilities)
	class UCharacterAbilitiesComponent* AbilityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat)
	class UCharacterCombatComponent* CombatComponent;

public:
	// Sets default values for this character's properties
	ABaseCharacter(const FObjectInitializer& ObjectInitializer);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// used to equip dropped weapons
	UPROPERTY()
	class AGunWeapon* OverlappingWeapon;

protected:
	// Overrides
	virtual void BeginPlay() override;

	// Movement
	void MoveForwardBackward(float Value);
	void ForwardDTPressed();
	void ForwardDTReleased();
	void ForwardDoubleTap();
	void MoveLeftRight(float Value);
	void StrafeDTPressedR();
	void StrafeDTPressedL();
	void StrafeDTReleased();
	void StrafeDoubleTap();
	void CustomJump();
	void SprintWhenAble();
	void Sprint();
	void SprintReleased();
	void SprintDoubleTapped();
	void CrouchSlideEnter();
	void CrouchSlideExit();

	// Aiming
	void LookPitch(float Value);
	void LookYaw(float Value);

	// Abilities
	void TacticalAbility();

public:
	// Combat
	void SetOverlappingWeapon(AGunWeapon* Weapon);

private:
	//
	// Movement
	//
	UPROPERTY(EditDefaultsOnly, Category = Movement) float DoubleTapTime;
	
	//
	// Aiming
	//
	UPROPERTY(EditDefaultsOnly, Category = Aiming) float AimSensitivity = 0.1f;
	UPROPERTY(EditDefaultsOnly, Category = Aiming) float ADSSensitivity = 0.05f;

public:
	//
	// Camera
	//

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* Camera;
	
	//
	// Input
	//
	
	//Double Tap Forward
	bool ForDTCanCheckPressed;
	bool ForDTCanCheckReleased;
	bool ForDTJustPressed;
	bool ForDTJustReleased;
	FTimerHandle ForwardDTPressedTimerHandle;
	FTimerHandle ForwardDTReleasedTimerHandle;	
	//Double Tap Strafe
	bool StrDTCanCheckPressedR;
	bool StrDTCanCheckPressedL;
	bool StrDTJustPressedR;
	bool StrDTJustPressedL;
	bool StrDTCanCheckReleased;
	bool StrDTJustReleased;
	FTimerHandle StrafeDTPressedRTimerHandle;
	FTimerHandle StrafeDTPressedLTimerHandle;
	FTimerHandle StrafeDTReleasedTimerHandle;
	
	//
	// Movement
	//

	// Base
	bool IsMovingForward;
	bool IsMovingBack;
	
	bool IsStrafing;
	bool bStrafeRight;

	float ForwardMulti;
	float BackMulti;
	float StrafeMulti;
	
	// Sprinting
	bool CheckToSprint;
	//bool IsSprinting;
	bool bSprintDoubleTap;
	//bool IsTacSprinting;
	FTimerHandle SprintDoubleTapTimerHandle;
	// Sliding
	//bool IsSliding;

	//
	// Getters
	//

	FCollisionQueryParams GetIgnoreCharacterParams() const;
	FORCEINLINE UTestMovementComponent* GetBaseCharacterMovement() const { return TestMovementComponent; }
	FORCEINLINE UCharacterAbilitiesComponent* GetBaseCharacterAbilities() const { return AbilityComponent; }
	
};
