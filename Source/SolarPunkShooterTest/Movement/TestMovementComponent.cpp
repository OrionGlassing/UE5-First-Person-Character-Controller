// Fill out your copyright notice in the Description page of Project Settings.


#include "TestMovementComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "SolarPunkShooterTest/Character/BaseCharacter.h"
#include "SolarPunkShooterTest/Character/CharacterAbilitiesComponent.h"

UTestMovementComponent::UTestMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bCanWalkOffLedges = true;
	bUseFlatBaseForFloorChecks = true;
	bUseSeparateBrakingFriction = true;
	
}

void UTestMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	BaseCharacterOwner = Cast<ABaseCharacter>(GetOwner());
	if (BaseCharacterOwner != nullptr)
	{
		BaseCharacterAbilities = Cast<UCharacterAbilitiesComponent>(BaseCharacterOwner->GetBaseCharacterAbilities());
	}
}

void UTestMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Set Boolean Values
	IsSprinting = false;
	IsTacSprinting = false;
	IsSliding = false;
	CanResetSprint = false;
	CanAutoChangeWalkSpeed = false;
	TacSprintDeplete = false;
	TacSprintOnCooldown = false;
	JumpLanded = false;
	IsJumping = false;
	CanLurch = false;
	SlideCooldownReset = true;
	SlideJumpCooldownReset = true;
	bEncroached = false;

	// Set Default Movement Values
	MaxWalkSpeed = Walk_MaxSpeed;
	GroundFriction = Default_GroundFriction; //not being used as far as i can tell
	BrakingFriction = Default_BrakingFriction;
	AirControl = AirControl_Min;

	GrappleConnectPoint = FVector::ZeroVector;
	GrappleCharStartPoint = FVector::ZeroVector;
	OriginalGrappleDirection = FVector::ZeroVector;
	
	BaseCharacterOwner->ForwardMulti = DefaultForwardMulti;
	BaseCharacterOwner->BackMulti = DefaultBackMulti;
	BaseCharacterOwner->StrafeMulti = DefaultStrafeMulti;
}

void UTestMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Set Updating Variables
	InputVector = BaseCharacterOwner->GetLastMovementInputVector(); // maybe change to last consumed inputvector
	
	//
	// Set Variables Based on Movement Mode
	//
	// When Falling (Not from a Jump), Set Air Movement Values
	if (IsFalling())
	{
		BrakingFriction = 0.f;
		GroundFriction = 0.f;
		UpdateAirControl();
	}
	// When back on the ground, Set Ground Movement Values
	if (IsWalking())
	{
		BrakingFriction = Default_BrakingFriction;
		GroundFriction = Default_GroundFriction;
		if (CanAutoChangeWalkSpeed)
		{
			if ((Velocity.Size() > Tac_MaxSpeed) && !IsTacSprinting)
			{
				EnterTacSprint();
			}
			else if ((Velocity.Size() > Sprint_MaxSpeed) && !IsSprinting)
			{
				Sprint();
			}
		}
		CanAutoChangeWalkSpeed = false;
	}
	else
	{
		CanAutoChangeWalkSpeed = true;
	}

	// Tick Reset Sprint (moved here from character)
	TickResetSprint();
	
	// Tick Tack Sprint Function
	TickTacSprint(DeltaTime);
	
	// Tick Slide Function
	TickSlide(DeltaTime);
	
	// Tick Wall Running Functions
	if (MovementMode != CMOVE_Wallrun)
	{
		EnterWallRun();
	}

	// Tick Jumping Function
	if (IsJumping)
	{
		TickJump(DeltaTime);
	}

	if (bEncroached)
	{
		ResetCapsuleSize();
	}
	
}

void UTestMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Play Mantle Root Motion
	if (bTransitionFinished_MY)
	{
		UE_LOG(LogTemp, Warning, TEXT("Applying MantleRMS!"));
		if (bTallMantle)
		{
			UE::Math::TVector2 InpRange = {-200.f, 200.f};
			UE::Math::TVector2 OutRange = {Mantle_MinMantleAnimSpeed * 3, Mantle_MinMantleAnimSpeed};
			MantleRMS->Duration = FMath::GetMappedRangeValueClamped(InpRange, OutRange, Velocity.Z);
			MantleRMS->StartLocation = UpdatedComponent->GetComponentLocation();
		}
		else
		{
			float MantleDistance = FVector::Dist(FinalMantlePosi, UpdatedComponent->GetComponentLocation());
			float ClampVel = FMath::Clamp(Velocity.Size(), Walk_MaxSpeed, 99999);
			MantleRMS->Duration = MantleDistance / ClampVel;
			MantleRMS->StartLocation = UpdatedComponent->GetComponentLocation();
		}
		Velocity = FVector::ZeroVector;
		BaseCharacterOwner->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		bMantleFinished_MY = false;
		MantleRMS_ID = ApplyRootMotionSource(MantleRMS);
		
		bTransitionFinished_MY = false;
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UTestMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
	// If we just had root motion source that ended, return to normal movement
    if (!HasRootMotionSources() && (bTransitionFinished_MY || bMantleFinished_MY) && IsFlying())
    {
        UE_LOG(LogTemp, Warning, TEXT("Ending Anim Root Motion!"));
        SetMovementMode(MOVE_Walking);
        IsMantling = false;
        Velocity.Z = 0.f;
        Acceleration.Z = 0.f;
    }
	if (GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ended TransitionRMS!"));
		RemoveRootMotionSourceByID(TransitionRMS_ID);
		Velocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
		bTransitionFinished_MY = true;
	}
	if (GetRootMotionSourceByID(MantleRMS_ID) && GetRootMotionSourceByID(MantleRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ended MantleRMS!"));
		RemoveRootMotionSourceByID(MantleRMS_ID);
		//Velocity = FVector::ZeroVector;
		Velocity.Z = 0.f;
		Acceleration.Z = 0.f;
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		BaseCharacterOwner->GetCapsuleComponent()->SetCollisionEnabled(
			DefaultCharacter->GetCapsuleComponent()->GetCollisionEnabled());
		bMantleFinished_MY = true;
		IsMantling = false;
	}
	
}

void UTestMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Wallrun:
		PhysWallRun(deltaTime, Iterations);
		break;
	case CMOVE_GrappleAbility:
		PhysGrappleAbility(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("Invalid Custom Movement Mode!"));
	}
}

float UTestMovementComponent::GetMaxSpeed() const
{
	if (MovementMode != MOVE_Custom) return Super::GetMaxSpeed();

	switch (CustomMovementMode)
	{
	case CMOVE_Wallrun:
		return MaxWallRunSpeed;
	case CMOVE_GrappleAbility:
		return Velocity.Size2D();
	default:
		UE_LOG(LogTemp, Warning, TEXT("Invalid Custom Movement Mode!"));
		return -1.f;
	}
}

float UTestMovementComponent::GetMaxAcceleration() const
{
	if (MovementMode != MOVE_Custom) return Super::GetMaxAcceleration();

	switch (CustomMovementMode)
	{
	case CMOVE_Wallrun:
		return WallRun_MaxMoveAcceleration;
	case CMOVE_GrappleAbility:
		if (BaseCharacterAbilities != nullptr) { return BaseCharacterAbilities->GrappleMoveMaxAcceleration; }
	default:
		UE_LOG(LogTemp, Warning, TEXT("Invalid Custom Movement Mode!"));
		return -1.f;
	}
}

void UTestMovementComponent::ForceResetMovement()
{
	UE_LOG(LogTemp, Warning, TEXT("Forced Reset Complete!"));
	MaxWalkSpeed = Walk_MaxSpeed;
	IsTacSprinting = false;
	TacSprintDeplete = false;
	IsSprinting = false;
	BaseCharacterOwner->ForwardMulti = DefaultForwardMulti;
	BaseCharacterOwner->StrafeMulti = DefaultStrafeMulti;
	BaseCharacterOwner->BackMulti = DefaultBackMulti;
}

void UTestMovementComponent::TickResetSprint()
{
	if (CanResetSprint && (MaxWalkSpeed != Walk_MaxSpeed) && (Velocity.Size2D() <= Walk_MaxSpeed && !BaseCharacterOwner->IsMovingForward))
	{
		UE_LOG(LogTemp, Warning, TEXT("Sprint Reset Complete!"));
		MaxWalkSpeed = Walk_MaxSpeed;
		IsTacSprinting = false;
		TacSprintDeplete = false;
		IsSprinting = false;
		BaseCharacterOwner->StrafeMulti = DefaultStrafeMulti;
		BaseCharacterOwner->BackMulti = DefaultBackMulti;
		CanResetSprint = false;
	}
}

bool UTestMovementComponent::Sprint()
{
	// only sprint if walking
	if (!IsWalking()) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Sprinting Now - TMC"));
	CanResetSprint = true;
	IsSprinting = true;
	BaseCharacterOwner->ForwardMulti = DefaultForwardMulti;
	BaseCharacterOwner->StrafeMulti = Sprint_StrafeMulti;
	BaseCharacterOwner->BackMulti = Sprint_BackMulti;
	MaxWalkSpeed = Sprint_MaxSpeed;
	
	return true;
}

void UTestMovementComponent::EnterTacSprint()
{
	if (IsWalking() && !TacSprintOnCooldown) {
		UE_LOG(LogTemp, Warning, TEXT("Tac Sprinting Now"));
		IsTacSprinting = true;
		// NOTE: Tac Sprint = true BUT Sprint = true ALSO
		MaxWalkSpeed = Tac_MaxSpeed;
		TacSprintOnCooldown = true;

		GetWorld()->GetTimerManager().SetTimer(
		TacSprintTimerHandle,
		this,
		&UTestMovementComponent::ExitTacSprint,
		TacSprintTime,
		false);
		
		GetWorld()->GetTimerManager().SetTimer(
			TacSprintCooldownHandle,
			this,
			&UTestMovementComponent::ResetTacSprint,
			TacSprintCooldownTime,
			false);
	}
}

void UTestMovementComponent::ExitTacSprint()
{
	//UE_LOG(LogTemp, Warning, TEXT("ExitTacSprint() Called"));
	if (IsTacSprinting)
	{
		TacSprintDeplete = true;
		bWantsToStopTacSprint = true;
		//UE_LOG(LogTemp, Warning, TEXT("ExitTacSprint() Executed"));
	}
}

void UTestMovementComponent::TickTacSprint(float DeltaTime)
{
	if (bWantsToStopTacSprint && MovementMode == MOVE_Walking)
	{
		if (TacSprintDeplete)
		{
			MaxWalkSpeed = FMath::FInterpTo(MaxWalkSpeed, Sprint_MaxSpeed, DeltaTime, TacSprintDepleteTime);
			if (Velocity.Size2D() < Sprint_MaxSpeed - 100.f)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Reset through velocity: %f"), BaseCharacterOwner->GetHorizontalVelocity());
				ForceResetMovement();
			}
			else if (MaxWalkSpeed < Sprint_MaxSpeed + 2.f)
			{
				//UE_LOG(LogTemp, Warning, TEXT("TickTacSprint() Finished"));
				Sprint();
				TacSprintDeplete = false;
				bWantsToStopTacSprint = false;
				IsTacSprinting = false;
			}
		}
	}
}

void UTestMovementComponent::ResetTacSprint()
{
	TacSprintOnCooldown = false;
	UE_LOG(LogTemp, Warning, TEXT("ResetTacSprint()"));
}

void UTestMovementComponent::EnterSlide()
{
	//only slide if we are falling or walking
	if (!IsFalling() && !IsWalking()) { return; }
	
	if (Velocity.Size2D() < Slide_MinVelocity) { return; }
	
	UE_LOG(LogTemp, Warning, TEXT("Start Slide"));
	HalfCapsuleSize();
	IsSliding = true;
	ForceResetMovement(); // Reset movement values to default so we always get same behavior + can start sprint after slide
	CanSlideBoost = true;
	if (IsMovingOnGround())
	{
		//Apply speed boost and clamp to maxspeedboost if we are on the floor when we start to slide
		if (SlideCooldownReset &&
			CalculateSlideDirection() != FVector::ZeroVector &&
			Velocity.Size() <= Slide_MaxBoostVelocity)
		{
			float const SlideBoost = FMath::InterpEaseInOut(Slide_MaxEnterBoost, 1.f,
				FMath::Clamp((Velocity.Size() / Slide_MaxBoostVelocity), 0.f, 1.f), 1.f);
			Velocity += CalculateSlideDirection()*Velocity.Size2D()*SlideBoost;
			if (Velocity.Size() > Slide_MaxBoostVelocity)
			{
				Velocity = Velocity.GetSafeNormal()*Slide_MaxBoostVelocity;
			}
			SlideCooldownReset = false;
			GetWorld()->GetTimerManager().SetTimer(
				SlideCooldownTimerHandle,
				this,
				&UTestMovementComponent::SlideCooldown,
				Slide_TimerCooldown,
				false);
		}
		CanSlideBoost = false;
	}
}

void UTestMovementComponent::TickSlide(float DeltaTime)
{
	//make sure we are sliding
	if (IsSliding)
	{
		//if we are on the ground
		if (IsMovingOnGround())
		{
			if (CanSlideBoost)
			{
				//Apply speed boost and clamp to maxspeedboost if we are on the floor when we start to slide
				if (SlideJumpCooldownReset &&
					CalculateSlideDirection() != FVector::ZeroVector &&
					Velocity.Size() <= Slide_MaxBoostVelocity)
				{
					float const SlideBoost = FMath::InterpEaseInOut(FMath::Clamp(Slide_MaxEnterBoost/2.f, 1.25f, 999.f), 1.f,
						FMath::Clamp((Velocity.Size() / Slide_MaxBoostVelocity), 0.f, 1.f), 1.f);
					Velocity += CalculateSlideDirection()*Velocity.Size2D()*SlideBoost;
					if (Velocity.Size() > Slide_MaxBoostVelocity)
					{
						Velocity = Velocity.GetSafeNormal()*Slide_MaxBoostVelocity;
					}
					SlideJumpCooldownReset = false;
					GetWorld()->GetTimerManager().SetTimer(
						SlideJumpCooldownTimerHandle,
						this,
						&UTestMovementComponent::SlideJumpCooldown,
						SlideJump_TimerCooldown,
						false);
				}
				else if (!SlideJumpCooldownReset)
				{
					SlideJumpCooldownReset = false;
					GetWorld()->GetTimerManager().SetTimer(
						SlideJumpCooldownTimerHandle,
						this,
						&UTestMovementComponent::SlideJumpCooldown,
						SlideJump_TimerCooldown,
						false);
				}
				CanSlideBoost = false;
			}
			UE_LOG(LogTemp, Warning, TEXT("Sliding -- ON the floor!"));
			//set friction anytime we are on the floor
			BrakingFriction = Slide_Friction;
			GroundFriction = Slide_GroundFriction;
			//set input scale once as we start slide on floor
			BaseCharacterOwner->ForwardMulti = SlideForwardMulti; 
			BaseCharacterOwner->BackMulti = SlideBackMulti;
			BaseCharacterOwner->StrafeMulti = SlideStrafeMulti;
			// Apply a downward force along the character down vector, mapped to the floor normal
			FVector Force = FVector::VectorPlaneProject(
				BaseCharacterOwner->GetActorUpVector(),
				CurrentFloor.HitResult.ImpactNormal)*Slide_GravityForce;
			Velocity -= Force * DeltaTime; // multiply by delta time so its framerate independent
			// Apply deceleration when the speed is lower than threshold
			if (Velocity.Size2D() < Slide_MaxBoostVelocity/2.f)
			{
				FVector Decel = Velocity.GetSafeNormal() * Slide_Deceleration;
				Decel.Z = 0.f;
				Velocity -= Decel * DeltaTime;
			}
			//check to exit slide (only exit here if on ground)
			if ((Velocity.Size2D() < Slide_MinVelocity) || (IsSprinting && Velocity.Size2D() < Sprint_MaxSpeed))
			{
				UE_LOG(LogTemp, Warning, TEXT("Exiting through conditions!!"));
				ExitSlide();
			}
		}
		else if (IsFalling())
		{
			UE_LOG(LogTemp, Warning, TEXT("Sliding -- NOT on the floor!"));
			//set friction anytime we are NOT on floor
			BrakingFriction = 0.f;
			GroundFriction = Default_GroundFriction;
		}
	}
}

void UTestMovementComponent::ExitSlide()
{
	/* Notes:
	 * Set friction back
	 * Reset input scale
	 * Set the capsule to normal height
	*/
	GroundFriction = Default_GroundFriction;
	if (IsMovingOnGround())
	{
		BrakingFriction = Default_BrakingFriction;
	}
	
	if (Velocity.Size() >= Sprint_MaxSpeed)
	{
		if (!Sprint())
		{
			ForceResetMovement();
		}
	}
	else
	{
		BaseCharacterOwner->ForwardMulti = DefaultForwardMulti;
		BaseCharacterOwner->StrafeMulti = DefaultStrafeMulti;
		BaseCharacterOwner->BackMulti = DefaultBackMulti;
	}
	
	ResetCapsuleSize();
	IsSliding = false;
	UE_LOG(LogTemp, Warning, TEXT("Finished exiting slide!!!"));
}

FVector UTestMovementComponent::CalculateSlideDirection()
{
	if (IsMovingOnGround() && Velocity.Size2D() != 0.f)
	{
		FVector FloorNormal = CurrentFloor.HitResult.ImpactNormal; 
		return FVector::VectorPlaneProject(Velocity.GetSafeNormal(), FloorNormal); 
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could NOT calculate slide direction!"));
		return FVector::ZeroVector;
	}
}

void UTestMovementComponent::SlideCooldown()
{
	SlideCooldownReset = true;
}

void UTestMovementComponent::SlideJumpCooldown()
{
	SlideJumpCooldownReset = true;
}

void UTestMovementComponent::EnterWallRun()
{
	if (!IsFalling()) { return; }
	if (Velocity.Size2D() < WallRun_MinSpeed) { return; }
	if (InputVector.IsNearlyZero()) { return; }
	//Left and Right raycasts to check for walls
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector LeftEnd = Start - UpdatedComponent->GetRightVector() * WallCheckDistance;
	FVector RightEnd = Start + UpdatedComponent->GetRightVector() * WallCheckDistance;
	auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
	FHitResult FloorHit, WallHit;
	//Check if player is high enough (if not then return)
	if (GetWorld()->LineTraceSingleByProfile(FloorHit, Start,
		Start + FVector::DownVector * (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + WallRun_MinHeight),
		"BlockAll", Params))
	{
		return;
	}
	// Check for Left Wall
	GetWorld()->LineTraceSingleByProfile(WallHit, Start, LeftEnd, "BlockAll", Params);
	if (WallHit.IsValidBlockingHit() && (Velocity | WallHit.Normal) < WallRun_MaxAngleDiff) 
	{
		// if we hit the wall, and our velocity is moving towards the wall, then the wallrun is left (not right)
		WallRunIsRight = false;
	}
	else
	{
		GetWorld()->LineTraceSingleByProfile(WallHit, Start, RightEnd, "BlockAll", Params);
		if (WallHit.IsValidBlockingHit() && (Velocity | WallHit.Normal) < WallRun_MaxAngleDiff)
		{
			// same as before but now wallrun is right
			WallRunIsRight = true;
		}
		else
		{
			// we dont have a valid wall on left or right, so cancel
			return;
		}
	}
	// if the wall is perpendicular to velocity rather than parallel, then exit
	if (FVector::DotProduct(Velocity.GetSafeNormal(), WallHit.ImpactNormal.GetSafeNormal()) > 0.75f)
	{
		return;
	}
	
	FVector ProjectedVelocity = FVector::VectorPlaneProject(Velocity, WallHit.Normal);
	if (ProjectedVelocity.Size2D() < WallRun_MinSpeed) { return; }

	//passed all conditions so start wall run
	IsWallRunning = true;
	BrakingFriction = WallRun_BrakingFriction;
	GroundFriction = 0.f;
	//Velocity = ProjectedVelocity; LOOK HERE
	
	//if we start with a negative z velocity, immediately half it.
	if (Velocity.Z < 0.f)
	{
		Velocity.Z /= 2.f;
	}
	//set max movement speed during wallrun
	if (Velocity.Size2D() < WallRun_MaxMoveSpeed)
	{
		MaxWallRunSpeed = WallRun_MaxMoveSpeed;
	}
	else
	{
		MaxWallRunSpeed = Velocity.Size2D();
	}
	SetMovementMode(MOVE_Custom, CMOVE_Wallrun);
	UE_LOG(LogTemp, Warning, TEXT("Starting Wall Run!!"));
}

void UTestMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{
	//Set Variables
	BrakingFriction = WallRun_BrakingFriction;
	GroundFriction = 0.f;
	// Boiler plate checks
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() &&
		!CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}
	bJustTeleported = false;
	float remainingTime = deltaTime;
	
	// Sub-Stepping Code (cool!)
	
	while ( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner &&
		(CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)) )
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		// Movement Code (fun!!)

		//get the wall we are running on
		FVector Start = UpdatedComponent->GetComponentLocation();
		FVector EndPointCalc = UpdatedComponent->GetRightVector() * WallCheckDistance;
		FVector End = WallRunIsRight ? Start + EndPointCalc : Start - EndPointCalc;
		auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
		float SinPullAwayAngle = FMath::Sin(FMath::DegreesToRadians(WallRun_PullAwayAngle));
		FHitResult WallHit;
		GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
		//check if we want to accelerate away from the wall
		bool bWantsToPullAway = WallHit.IsValidBlockingHit() &&
			!Acceleration.IsNearlyZero() &&
				(Acceleration.GetSafeNormal() | WallHit.Normal) > SinPullAwayAngle;
		//if there is no wall, OR we are accelerating away from the wall --> exit wall run
		if (!WallHit.IsValidBlockingHit() || bWantsToPullAway)
		{
			UE_LOG(LogTemp, Warning, TEXT("Exiting Wall Run: NO WALL / PULLIGN AWAY"));
			ExitWallRun();
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		// if the wall is perpendicular to velocity rather than parallel, then exit
		if (FVector::DotProduct(Velocity.GetSafeNormal(), WallHit.ImpactNormal.GetSafeNormal()) > 0.75f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Exiting Wall Run: WALL PERPENDICULAR"));
			ExitWallRun();
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		//clamp acceleration
		Acceleration = FVector::VectorPlaneProject(Acceleration, WallHit.Normal);
		Acceleration.Z = 0.f;
		//apply acceleration
		CalcVelocity(timeTick, 0.f, false, 0.f); // this might be all we need to ignore gravity
		
		Velocity = FVector::VectorPlaneProject(Velocity, WallHit.Normal);
		
		//apply new gravity
		if (Velocity.Z > 50.f)
		{
			Velocity.Z += GetGravityZ() * timeTick;
		}
		else
		{
			Velocity.Z += WallRun_SlippingGravityConstant * timeTick;
			Velocity.Z += Velocity.Z * WallRun_SlippingGravityMultiplier * timeTick;
		}
		//exit if too slow
		if (Velocity.Size2D() < WallRun_MinSpeed)
		{
			UE_LOG(LogTemp, Warning, TEXT("Exiting Wall Run: TOO SLOW"));
			ExitWallRun();
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		// Compute move parameters
		const FVector Delta = timeTick * Velocity; // dx = v * dt
		const bool bZeroDelta = Delta.IsNearlyZero();
		if ( bZeroDelta )
		{
			remainingTime = 0.f;
		}
		else
		{
			FHitResult Hit;
			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
			// if we hit something, handle that hit and slide along surface
			if (Hit.Time < 1.f)
			{
				HandleImpact(Hit, timeTick, Delta); // maybe try deltaTime
				SlideAlongSurface(Delta, (1.f - Hit.Time), Hit.Normal, Hit, true);
			}
			// apply wall attraction force
			FVector WallAttractionDelta = -WallHit.Normal * WallAttractionForce * timeTick;
			SafeMoveUpdatedComponent(WallAttractionDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
			// if we hit something, handle that hit
			HandleImpact(Hit, timeTick, Delta); // maybe try deltaTime
		}
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick; // v = dx / dt
	} // end of substepping loop
	
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector EndPointCalc = UpdatedComponent->GetRightVector() * WallCheckDistance;
	FVector End = WallRunIsRight ? Start + EndPointCalc : Start - EndPointCalc;
	auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
	float SinPullAwayAngle = FMath::Sin(FMath::DegreesToRadians(WallRun_PullAwayAngle));
	FHitResult WallHit, FloorHit;
	GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
	GetWorld()->LineTraceSingleByProfile(FloorHit, Start,
		Start+FVector::DownVector * (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + WallRun_MinHeight),
		"BlockAll", Params);
	//if too close to floor, no wall, or too slow --> exit wallrun
	if (FloorHit.IsValidBlockingHit())
	{
		UE_LOG(LogTemp, Warning, TEXT("Exiting Wall Run: TOO LOW TO GROUND"));
		ExitWallRun();
	}
	else if (!WallHit.IsValidBlockingHit())
	{
		UE_LOG(LogTemp, Warning, TEXT("Exiting Wall Run: NO WALL 2"));
		ExitWallRun();
	}
	else if (Velocity.Size2D() < WallRun_MinSpeed)
	{
		UE_LOG(LogTemp, Warning, TEXT("Exiting Wall Run: TOO SLOW 2"));
		ExitWallRun();
		
	}
}

void UTestMovementComponent::ExitWallRun()
{
	IsWallRunning = false;
	SetMovementMode(MOVE_Falling);
	BrakingFriction = Default_BrakingFriction;
	GroundFriction = Default_GroundFriction;
	if (Velocity.Size2D() > Walk_MaxSpeed)
	{
		Sprint();
	}
	UE_LOG(LogTemp, Warning, TEXT("Finished exiting Wall Run!!!"));
}

//
// Mantle 
//

bool UTestMovementComponent::TryMantle()
{
	if (!IsFalling() && !IsWalking()) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Beginning to Attempt Mantle!!"));
	//setup variables
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector Fwd = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2 + Mantle_ReachHeight;
	//CosMMWSA
	float CosMinWallSteepness = FMath::Cos(FMath::DegreesToRadians(Mantle_MinWallSteepnessAngle));
	//CosMMSA
	float CosMaxSurface = FMath::Cos(FMath::DegreesToRadians(Mantle_MaxSurfaceAngle));
	//CosMMAA
	float CosMaxAlignment = FMath::Cos(FMath::DegreesToRadians(Mantle_MaxAlignmentAngle));
	//Check Front Face
	FHitResult FrontHit;
	UE::Math::TVector2 InpRange = {0.f, Tac_MaxSpeed};
	UE::Math::TVector2 OutRange = {Mantle_MaxDistance/3, Mantle_MaxDistance};
	float CheckDistance = FMath::GetMappedRangeValueClamped(InpRange, OutRange, Velocity | Fwd);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);
	//send a bunch of line traces
	for (int i = 0; i < 6; i ++)
	{
		if (GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + Fwd*CheckDistance, "BlockAll", Params)) { break; }
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / 5;
	}
	if (!FrontHit.IsValidBlockingHit()) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Mantle: Passed 1"));
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	//if wall too steep, or player not looking towards wall -> exit
	if (FMath::Abs(CosWallSteepnessAngle) > CosMinWallSteepness || (Fwd | -FrontHit.Normal) < CosMinWallSteepness) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Mantle: Passed 2"));
	//Check Height
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + Fwd + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
	if (!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + Fwd, "BlockAll", Params)) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Mantle: Passed 3"));
	for (const FHitResult& Hit : HeightHits)
	{
		if (Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;
		}
	}
	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosMaxSurface) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Mantle: Passed 4"));
	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;
	if (Height > MaxHeight) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Mantle: Passed 5"));
	//Check Clearance
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector ClearCapLoc = SurfaceHit.Location + Fwd * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	if (GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params)) { return false; }
	UE_LOG(LogTemp, Warning, TEXT("Mantle: Passed 6"));

	// All Conditions Met! Time to Enter Mantle!

	//if sliding, stop
	if (IsSliding)
	{
		ExitSlide();
	}
	
	ForceResetMovement();
	UE_LOG(LogTemp, Warning, TEXT("Good to go, starting mantle!!!"));
	//Mantle Selection
	FVector ShortMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, false);
	FVector TallMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, true);
	// tall mantle if wall is taller than character, otherwise short mantle
	bTallMantle = false;
	if (Height > CapHH() * 2) { bTallMantle = true; }

	/* if falling and moving down, do tall mantle
	if (IsFalling() && (Velocity | FVector::UpVector()) < 0.f)
	{
		// only tall mantle if the bottom of the mantle is NOT obstructed and we can
		// fit an entire capsule into the tall mantle start position
		if (!GetWorld()->OverlapAnyTestByProfile(TallMantleTarget, FQuat::Identity, "BlockAll", CapShape, Params)) { bTallMantle = true; }
	}
	*/
	FVector TransitionTarget = bTallMantle ? TallMantleTarget : ShortMantleTarget;

	if (bTallMantle)
	{
		UE_LOG(LogTemp, Warning, TEXT("Using Tall Mantle!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Using Short Mantle!"));
	}
	//Transition to Mantle (look here to change hard constant values)
	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	// V = d / t --> t = d / V
	TransitionRMS->Duration = TransDistance / (Velocity.Size() + JumpZVelocity);
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	//Apply Root Motion Transition Source
	UE_LOG(LogTemp, Warning, TEXT("Applying TransitionRMS!"));
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	bTransitionFinished_MY = false;
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);

	// Setup Constants of MantleRMS
	MantleRMS.Reset();
	MantleRMS = MakeShared<FRootMotionSource_MoveToForce>();
	MantleRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	MantleRMS->TargetLocation = ClearCapLoc;
	FinalMantlePosi = ClearCapLoc;
	
	IsMantling = true;
	return true;
}

FVector UTestMovementComponent::GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit,
	bool isTallMantle) const
{
	// calculate where the capsule should be when the mantle animation starts
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	float DownDistance = isTallMantle ? CapHH() * 1.5f : MaxStepHeight - 1;
	FVector EdgeTangent = FVector::CrossProduct(SurfaceHit.Normal, FrontHit.Normal).GetSafeNormal();

	FVector MantleStart = SurfaceHit.Location;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * (2.f + CapR());
	MantleStart += UpdatedComponent->GetForwardVector().GetSafeNormal2D().ProjectOnTo(EdgeTangent) * CapR() * .3f;
	MantleStart += FVector::UpVector * CapHH();
	MantleStart += FVector::DownVector * DownDistance;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * CosWallSteepnessAngle * DownDistance;

	return MantleStart;
}

bool UTestMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || IsWallRunning;
}

void UTestMovementComponent::StartJump()
{
	//save wallrun state
	bool WasWallRunning = IsWallRunning;
	//check if we can mantle, if we can don't jump, if we can't then do jump
	if (!IsMantling && TryMantle())
	{
		//we just mantled so that's cool...
		return;
	}
	else
	{
		if (DoJump(false))
		{
        		//If Wall Running --> Add a force away from wall!
        		if (WasWallRunning)
        		{
        			ExitWallRun();
        			FVector Start = UpdatedComponent->GetComponentLocation();
        			FVector EndPointCalc = UpdatedComponent->GetRightVector() * WallCheckDistance;
        			FVector End = WallRunIsRight ? Start + EndPointCalc : Start - EndPointCalc;
        			auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
        			FHitResult WallHit;
        			GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
        			UE::Math::TVector2 InRange = {WallRun_MinSpeed, WallRun_MaxMoveSpeed};
        			UE::Math::TVector2 OutRange = {0.f, WallJumpOffForce};
        			float JumpBoost = FMath::GetMappedRangeValueClamped(InRange, OutRange, Velocity.Size());
        			Velocity += WallHit.Normal * JumpBoost;
        			UE_LOG(LogTemp, Warning, TEXT("Special Jump - Wall Run Jump!"));
        		}
        		//Set movement values and variables
        		IsJumping = true;
        		BrakingFriction = 0.f;
        		JumpLanded = false;
        		CanLurch = true;
        		BaseCharacterOwner->ForwardMulti = DefaultForwardMulti;
        		BaseCharacterOwner->StrafeMulti = DefaultStrafeMulti;
        		BaseCharacterOwner->BackMulti = DefaultBackMulti;
				//Enters Tick Jumping Function
		}
	}
}

void UTestMovementComponent::TickJump(float DeltaTime)
{
	//Check for Jump Landing
	if (!IsFalling()) {
		JumpLanded = true;
		EndJump();
		//UE_LOG(LogTemp, Warning, TEXT("Landed"));
	}
}

void UTestMovementComponent::EndJump()
{
	IsJumping = false;
	CanLurch = false;
	UE_LOG(LogTemp, Warning, TEXT("Jump Ended"));
	if (IsSliding)
	{
		BrakingFriction = Slide_Friction;
	}
	else
	{
		BrakingFriction = Default_BrakingFriction;
	}
	if (IsSprinting)
	{
		BaseCharacterOwner->StrafeMulti = Sprint_StrafeMulti;
		BaseCharacterOwner->BackMulti = Sprint_BackMulti;
	}
	AirControl = AirControl_Min;
	//UE_LOG(LogTemp, Warning, TEXT("Not jumping anymore"));
}

void UTestMovementComponent::Lurch(FVector InputVectorPending)
{
	UE_LOG(LogTemp, Warning, TEXT("Input Vector Size: %f"), InputVectorPending.Size2D());
	UE_LOG(LogTemp, Warning, TEXT("Dot Product Result: %f"),
		FVector::DotProduct(InputVectorPending.GetSafeNormal2D(), Velocity.GetSafeNormal2D()));
	
	if (CanLurch
		&& InputVectorPending.Size2D() != 0.f
		&& FVector::DotProduct(InputVectorPending.GetSafeNormal2D(), Velocity.GetSafeNormal2D()) <= LurchMinAngle
		&& FVector::DotProduct(InputVectorPending.GetSafeNormal2D(), Velocity.GetSafeNormal2D()) >= LurchMaxAngle
		)
	{
		float const VelSize = Velocity.Size2D();
		Velocity.X = 0.f;
		Velocity.Y = 0.f;
		InputVectorPending.Z = 0.f;
		Velocity += InputVectorPending.GetSafeNormal2D() * VelSize * LurchMultiplier;
		CanLurch = false;
		UE_LOG(LogTemp, Warning, TEXT("We have lurched!"));
	}
}

void UTestMovementComponent::UpdateAirControl()
{
	UE::Math::TVector2 const InputRange = {-1.f, 1.f};
	UE::Math::TVector2 const OutputRange = {AirControl_Max, AirControl_Min};
	UE::Math::TVector2 const InputRange2 = {0.f, Walk_MaxSpeed};
	UE::Math::TVector2 const OutputRange2 = {AirControl_Max, 0.f};
	AirControlBoost = FMath::GetMappedRangeValueClamped(InputRange2, OutputRange2, Velocity.Size2D());
	AirControl = FMath::GetMappedRangeValueClamped(InputRange, OutputRange,
		FVector::DotProduct(InputVector.GetSafeNormal2D(), Velocity.GetSafeNormal2D())) + AirControlBoost;
}
//
// Grapple Ability Movement
//

void UTestMovementComponent::EnterGrapple()
{
	if (BaseCharacterAbilities == nullptr) { return; }
	if (GrappleConnectPoint == FVector::ZeroVector) { return; }
	if (OriginalGrappleDirection == FVector::ZeroVector) { return; }
	if (GrappleCharStartPoint == FVector::ZeroVector) { return; }
	IsGrappling = true;
	bCanDisconnect = false;
	bVelocityWasGreaterGrapple = false;
	BrakingFriction = 0.f;
	GroundFriction = 0.f;
	//grapple disconnect point calculations
	GrappleAutoDisconnectPoint = GrappleConnectPoint + (OriginalGrappleDirection * BaseCharacterAbilities->GrappleDisconnectDistance);
	SetMovementMode(MOVE_Custom, CMOVE_GrappleAbility);
	UE_LOG(LogTemp, Warning, TEXT("Movement Component --> Grapple: Starting Grapple!"));
}

void UTestMovementComponent::PhysGrappleAbility(float deltaTime, int32 Iterations)
{
	//Extra Checks
	if (BaseCharacterAbilities == nullptr) { return; }
	if (GrappleConnectPoint == FVector::ZeroVector) { return; }
	if (OriginalGrappleDirection == FVector::ZeroVector) { return; }
	if (GrappleCharStartPoint == FVector::ZeroVector) { return; }
	
	//Set Variables
	BrakingFriction = 0.f;
	GroundFriction = 0.f;
	
	// Boiler plate checks
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() &&
		!CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}
	bJustTeleported = false;
	float remainingTime = deltaTime;
	
	// Sub-Stepping Code (cool!)
	
	while ( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner &&
		(CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)) )
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		// Movement Code (fun!!)

		if (Velocity.Size() > BaseCharacterAbilities->GrappleMinSpeed)
		{
			bVelocityWasGreaterGrapple = true;
		}
		
		//find angle between position and grapple point
		FVector Difference = (GrappleConnectPoint - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
		
		//check if there is anything in between the player and the grapple
		FVector Start = BaseCharacterOwner->Camera->GetComponentLocation();
		FVector End = GrappleConnectPoint - (Difference * 100.f);
		auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
		FHitResult GrappleHit;
		GetWorld()->LineTraceSingleByProfile(GrappleHit, Start, End, "BlockAll", Params);

		if (GrappleHit.IsValidBlockingHit())
		{
			UE_LOG(LogTemp, Warning, TEXT("Exiting Grapple: Grapple Obstructed!"));
			ExitGrapple();
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		
		// Calculations between Position and Grapple Point

		//if too close to grapple connect point, exit
		float Distance = FVector::Distance(UpdatedComponent->GetComponentLocation(), GrappleConnectPoint);
		if ((Distance < BaseCharacterAbilities->GrappleMinDistance) || (Distance > BaseCharacterAbilities->GrappleReachDistance * 1.5))
		{
			UE_LOG(LogTemp, Warning, TEXT("Exiting Grapple: Too Close / Far to Grapple"));
			ExitGrapple();
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		//calculate point along grapple path
		FVector CurrentPlace = UpdatedComponent->GetComponentLocation();
		float disconnectDelta = ((CurrentPlace - GrappleCharStartPoint) | OriginalGrappleDirection);
		FVector ProjectedPoint = GrappleCharStartPoint + (OriginalGrappleDirection * disconnectDelta);
		//draw projected point
		DrawDebugSphere(GetWorld(), ProjectedPoint, 25.f, 12, FColor::Purple, false, -1, 0, 2);
		//calculate if we are looking towards the grapple
		bool LookingAtGrapple = ((BaseCharacterOwner->Camera->GetComponentRotation().Vector().GetSafeNormal() |
			Difference.GetSafeNormal()) > 0.55f);
		if (FVector::PointsAreNear(ProjectedPoint, GrappleAutoDisconnectPoint, 10))
		{
			bCanDisconnect = true;
		}
		if (bCanDisconnect && !LookingAtGrapple)
		{
			UE_LOG(LogTemp, Warning, TEXT("Exiting Grapple: Auto Disconnect Point"));
			ExitGrapple();
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		//draw disconnect point
		DrawDebugSphere(GetWorld(), GrappleAutoDisconnectPoint, 25.f, 12, FColor::Magenta, false, -1, 0, 2);

		// clamp / project acceleration & input
		/*
		Acceleration -= (Difference * Acceleration);
		float AccelDotProduct = FVector::DotProduct(Acceleration.GetSafeNormal2D(), Difference.GetSafeNormal2D());
		UE::Math::TVector2 const InputRange2 = {0.f, 1.f};
		UE::Math::TVector2 const OutputRange2 = {AirControl_Max, 0.f};
		float AccelerationScale = FMath::InterpEaseInOut(1.f, 0.f, FMath::Abs(AccelDotProduct), 4);
		Acceleration = Acceleration * AccelerationScale;
		*/
		
		//line showing acceleration direction
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentLocation() + (Acceleration),
			FColor::Blue,
			false,
			-1,
			0,
			4);

		// apply grapple attraction force
		float GrapplePull = FMath::InterpEaseOut(BaseCharacterAbilities->GrapplePullAcceleration, 0.1f,
			FMath::Clamp((Velocity.Size()/BaseCharacterAbilities->GrappleMaxSpeed), 0.f, 1.f), 2);
		FVector GrappleAttractionDelta = Difference * GrapplePull * timeTick;
		Velocity += GrappleAttractionDelta;
		
		CalcVelocity(timeTick, 0.f, false, 0.f); // this might be all we need to ignore gravity

		//line showing velocity direction
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentLocation() + (Velocity),
			FColor::Green,
			false,
			-1,
			0,
			4);
		
		// Compute move parameters
		const FVector Delta = timeTick * Velocity; // dx = v * dt
		const bool bZeroDelta = Delta.IsNearlyZero();
		if ( bZeroDelta )
		{
			remainingTime = 0.f;
		}
		else
		{
			FHitResult Hit;
			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
			// if we hit something, handle that hit and slide along surface
			if (Hit.Time < 1.f)
			{
				HandleImpact(Hit, timeTick, Delta); // maybe try deltaTime
				SlideAlongSurface(Delta, (1.f - Hit.Time), Hit.Normal, Hit, true);
			}
		}
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick; // v = dx / dt

		// Draw Debug Visuals

		//grapple point
		DrawDebugSphere(GetWorld(), GrappleConnectPoint, 25.f, 12, FColor::Red, false, -1, 0, 2);
		//line between character and grapple point
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), GrappleConnectPoint,
			FColor::Red,
			false,
			-1,
			0,
			2);
		
	} // end of substepping loop

	//exit if too slow
	if (Velocity.Size() < BaseCharacterAbilities->GrappleMinSpeed && bVelocityWasGreaterGrapple)
	{
		UE_LOG(LogTemp, Warning, TEXT("Exiting Grapple: Too Slow!"));
		ExitGrapple();
	}
}

void UTestMovementComponent::ExitGrapple()
{
	if (BaseCharacterAbilities != nullptr)
	{
		BaseCharacterAbilities->StartGrappleCooldown();
	}
	IsGrappling = false;
	GrappleConnectPoint = FVector::ZeroVector;
	GrappleCharStartPoint = FVector::ZeroVector;
	OriginalGrappleDirection = FVector::ZeroVector;
	SetMovementMode(MOVE_Falling);
	ForceResetMovement();
	BrakingFriction = Default_BrakingFriction;
	GroundFriction = Default_GroundFriction;
	if (Velocity.Size2D() > Walk_MaxSpeed)
	{
		Sprint();
	}
	UE_LOG(LogTemp, Warning, TEXT("Finished Exiting Grapple!!!"));
}

//
// Adjust Capsule Size
//

void UTestMovementComponent::HalfCapsuleSize()
{
	if (!HasValidData()) { return; }
	// See if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == CrouchedHalfHeight) { return; }
	// Create a default character to compare values to
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	// restore collision size before changing capsule size
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(BaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),
		DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	// Change collision size to crouching dimensions
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	// Height is not allowed to be smaller than radius.
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, CrouchedHalfHeight);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedCrouchedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedCrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	const float MeshAdjust = ScaledHalfHeightAdjust;
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ClampedCrouchedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );
}

void UTestMovementComponent::ResetCapsuleSize()
{
	if (!HasValidData()) { return; }
	// Create a default character to compare values to
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	// See if collision is already at desired size.
	if( CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() ==
		DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() )
	{
		UE_LOG(LogTemp, Warning, TEXT("Value is ALREADY desired size!"))
		return;
	}
	
	const float CurrentCrouchedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
	// Grow to uncrouched size.
	check(CharacterOwner->GetCapsuleComponent());

	// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		const UWorld* MyWorld = GetWorld();
		const float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
		FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);

		// Compensate for the difference between current capsule size and standing size
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust); // Shrink by negative amount, so actually grow it.
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		bEncroached = true;

		if (!bCrouchMaintainsBaseLocation)
		{
			// Expand in place
			bEncroached = MyWorld->OverlapBlockingTestByChannel(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
		
			if (bEncroached)
			{
				// Try adjusting capsule position to see if we can avoid encroachment.
				if (ScaledHalfHeightAdjust > 0.f)
				{
					// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
					float PawnRadius, PawnHalfHeight;
					CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
					const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
					const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
					const FVector Down = FVector(0.f, 0.f, -TraceDist);

					FHitResult Hit(1.f);
					const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
					const bool bBlockingHit = MyWorld->SweepSingleByChannel(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
					if (Hit.bStartPenetrating)
					{
						bEncroached = true;
					}
					else
					{
						// Compute where the base of the sweep ended up, and see if we can stand there
						const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
						const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + StandingCapsuleShape.Capsule.HalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
						bEncroached = MyWorld->OverlapBlockingTestByChannel(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
						if (!bEncroached)
						{
							// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
							UpdatedComponent->MoveComponent(NewLoc - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
						}
					}
				}
			}
		}
		else
		{
			// Expand while keeping base location the same.
			FVector StandingLocation = PawnLocation + FVector(0.f, 0.f, StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight);
			bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				if (IsMovingOnGround())
				{
					// Something might be just barely overhead, try moving down closer to the floor to avoid it.
					const float MinFloorDist = UE_KINDA_SMALL_NUMBER * 10.f;
					if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
					{
						StandingLocation.Z -= CurrentFloor.FloorDist - MinFloorDist;
						bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
					}
				}				
			}

			if (!bEncroached)
			{
				// Commit the change in location.
				UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
				bForceNextFloorCheck = true;
			}
		}

		// If still encroached then abort.
		if (bEncroached)
		{
			UE_LOG(LogTemp, Warning, TEXT("We are Encroached!"));
			return;
		}
	// Now call SetCapsuleSize() to cause touch/untouch events and actually grow the capsule
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(BaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),
		DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);

	const float MeshAdjust = ScaledHalfHeightAdjust;
	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );
	UE_LOG(LogTemp, Warning, TEXT("We are have UNcrouched successfullay!"));
}

float UTestMovementComponent::CapR() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float UTestMovementComponent::CapHH() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}


