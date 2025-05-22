// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TestMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None				UMETA(Hidden),
	CMOVE_Wallrun			UMETA(DisplayName = "Wall Run"),
	CMOVE_GrappleAbility	UMETA(DisplayName = "Grapple Ability"),
	CMOVE_MAX				UMETA(Hidden),
};

UCLASS()
class SOLARPUNKSHOOTERTEST_API UTestMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UTestMovementComponent();

protected:
	// Called when the game starts
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

public:
	class ABaseCharacter* BaseCharacterOwner;
	class UCharacterAbilitiesComponent* BaseCharacterAbilities;
	
	// Base Variables
	FVector InputVector;
	bool CanResetSprint;
	bool CanAutoChangeWalkSpeed;

	// Sprint
	bool IsSprinting;

	// TacSprint
	FTimerHandle TacSprintTimerHandle;
	FTimerHandle TacSprintCooldownHandle;
	bool IsTacSprinting;
	bool TacSprintDeplete;
	bool TacSprintOnCooldown;
	bool bWantsToStopTacSprint;

	// Slide
	bool IsSliding;
	bool SlideCooldownReset;
	bool SlideJumpCooldownReset;
	bool CanSlideBoost;
	FTimerHandle SlideCooldownTimerHandle;
	FTimerHandle SlideJumpCooldownTimerHandle;

	// Wall Run
	bool IsWallRunning;
	bool WallRunIsRight;
	float MaxWallRunSpeed;

	// Mantle
	bool IsMantling;
	bool bTallMantle;
	FVector FinalMantlePosi;
	bool bTransitionFinished_MY;
	bool bMantleFinished_MY;
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	int TransitionRMS_ID;
	TSharedPtr<FRootMotionSource_MoveToForce> MantleRMS;
	int MantleRMS_ID;

	// Jump
	bool IsJumping;
	bool JumpLanded;
	bool CanLurch;
	float AirControlBoost;

	// Grapple Ability
	bool IsGrappling;
	bool bCanDisconnect;
	bool bVelocityWasGreaterGrapple;
	FVector GrappleConnectPoint;
	FVector GrappleCharStartPoint;
	FVector OriginalGrappleDirection;
	FVector GrappleAutoDisconnectPoint;

	// Capsule
	bool bEncroached;
	
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	//
	// Variables
	//

	// Base Info
	UPROPERTY(EditDefaultsOnly, Category = BaseInfo) float Default_GroundFriction;
	UPROPERTY(EditDefaultsOnly, Category = BaseInfo) float Default_BrakingFriction;

	// Strafing Speeds
	UPROPERTY(EditDefaultsOnly, Category = MovementMulti) float DefaultForwardMulti;
	UPROPERTY(EditDefaultsOnly, Category = MovementMulti) float DefaultBackMulti;
	UPROPERTY(EditDefaultsOnly, Category = MovementMulti) float DefaultStrafeMulti;
	UPROPERTY(EditDefaultsOnly, Category = MovementMulti) float SlideForwardMulti;
	UPROPERTY(EditDefaultsOnly, Category = MovementMulti) float SlideBackMulti;
	UPROPERTY(EditDefaultsOnly, Category = MovementMulti) float SlideStrafeMulti;
	
	// Sprint
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float Walk_MaxSpeed;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float Sprint_MaxSpeed;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float Sprint_StrafeMulti;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float Sprint_BackMulti;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float Tac_MaxSpeed;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float TacSprintTime;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float TacSprintDepleteTime;
	UPROPERTY(EditDefaultsOnly, Category = Sprint) float TacSprintCooldownTime;
	
	// Slide
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_MinVelocity;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_MaxBoostVelocity;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_GravityForce;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_Friction;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_Deceleration;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_GroundFriction;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_MaxEnterBoost;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float Slide_TimerCooldown;
	UPROPERTY(EditDefaultsOnly, Category = Slide) float SlideJump_TimerCooldown;

	// On Wall General
	UPROPERTY(EditDefaultsOnly, Category = OnWall) float WallJumpOffForce;
	UPROPERTY(EditDefaultsOnly, Category = OnWall) float WallAttractionForce;
	UPROPERTY(EditDefaultsOnly, Category = OnWall) float WallCheckDistance;
	//Wall Run
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_MinSpeed;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_MinHeight;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_MaxAngleDiff;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_PullAwayAngle;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_MaxMoveSpeed;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_MaxMoveAcceleration;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_BrakingFriction;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_SlippingGravityConstant;
	UPROPERTY(EditDefaultsOnly, Category = WallRun) float WallRun_SlippingGravityMultiplier;

	//Mantle
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_MaxDistance;
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_ReachHeight;
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_Depth;
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_MinWallSteepnessAngle;
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_MaxSurfaceAngle;
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_MaxAlignmentAngle;
	UPROPERTY(EditDefaultsOnly, Category = Mantle) float Mantle_MinMantleAnimSpeed;

	// Jump
	UPROPERTY(EditDefaultsOnly, Category = Jump) float AirControl_Max;
	UPROPERTY(EditDefaultsOnly, Category = Jump) float AirControl_Min;
	UPROPERTY(EditDefaultsOnly, Category = Jump) float LurchMultiplier;
	UPROPERTY(EditDefaultsOnly, Category = Jump) float LurchMinAngle;
	UPROPERTY(EditDefaultsOnly, Category = Jump) float LurchMaxAngle;

	//
	// Functions
	//
	
	// Misc
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	void ForceResetMovement();
	void TickResetSprint();
	//Sprint
	bool Sprint();
	void EnterTacSprint();
	void ExitTacSprint();
	void TickTacSprint(float DeltaTime);
	void ResetTacSprint();
	//Slide
	void EnterSlide();
	void TickSlide(float DeltaTime);
	void ExitSlide();
	FVector CalculateSlideDirection();
	void SlideCooldown();
	void SlideJumpCooldown();
	//WallRun
	void EnterWallRun();
	void PhysWallRun(float deltaTime, int32 Iterations);
	void ExitWallRun();
	//Mantle
	bool TryMantle();
	FVector GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool isTallMantle) const;
	//Jump
	virtual bool CanAttemptJump() const override;
	void StartJump();
	void TickJump(float DeltaTime);
	void EndJump();
	void Lurch(FVector InputVectorPending);
	void UpdateAirControl();
	//Grapple
	void EnterGrapple();
	void PhysGrappleAbility(float deltaTime, int32 Iterations);
	void ExitGrapple();
	//Capsule
	void HalfCapsuleSize();
	void ResetCapsuleSize();
	float CapR() const;
	float CapHH() const;
};
