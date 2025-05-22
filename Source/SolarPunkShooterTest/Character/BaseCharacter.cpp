// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"

#include "CharacterAbilitiesComponent.h"
#include "SceneRenderTargetParameters.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "SolarPunkShooterTest/Movement/TestMovementComponent.h"
#include "SolarPunkShooterTest/Weapons/GunWeapon.h"


// Sets default values
ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTestMovementComponent>(CharacterMovementComponentName))
{
	// setup components
	TestMovementComponent = Cast<UTestMovementComponent>(GetCharacterMovement());
	AbilityComponent = CreateDefaultSubobject<UCharacterAbilitiesComponent>(TEXT("AbilityComponent"));
	
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// setup camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 2;
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = true;
	
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Movement
	IsMovingForward = false;
	IsMovingBack = false;
	IsStrafing = false;
	IsSprinting = false;
	bSprintDoubleTap = false;
	IsTacSprinting = false;
	IsSliding = false;
	Jumped = false;
	ForDTCanCheckPressed = true;
	ForDTCanCheckReleased = true;
	IsCrouching = false;
	
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Tick Sprint Function
	if (CheckToSprint)
	{
		SprintWhenAble();
	}
	
	//MovementCameraFOV(DeltaTime);
}

//
// Player Input
//

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward/Backward", this, &ABaseCharacter::MoveForwardBackward);
	PlayerInputComponent->BindAxis("MoveLeft/Right", this, &ABaseCharacter::MoveLeftRight);
	PlayerInputComponent->BindAxis("Pitch", this, &ABaseCharacter::LookPitch);
	PlayerInputComponent->BindAxis("Yaw", this, &ABaseCharacter::LookYaw);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABaseCharacter::CustomJump);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ABaseCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ABaseCharacter::SprintReleased);
	PlayerInputComponent->BindAction("Crouch/Slide", IE_Pressed, this, &ABaseCharacter::CrouchSlideEnter);
	PlayerInputComponent->BindAction("Crouch/Slide", IE_Released, this, &ABaseCharacter::CrouchSlideExit);
	PlayerInputComponent->BindAction("TacticalAbility", IE_Pressed, this, &ABaseCharacter::TacticalAbility);
}

//
// MOVEMENT
//

// NOTE: I just went back to the OLD implementation
// Right now double tap is tripped and stays true for multiple frames
// this didn't seem to fix our original issue of forward double tap not registering a lurch
// BUT it might have made it better...

void ABaseCharacter::MoveForwardBackward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		FVector const Direction = GetActorForwardVector();
		if (Value > 0.f)
		{
			IsMovingForward = true;
			AddMovementInput(Direction, Value*ForwardMulti);
			if (!ForDTJustReleased)
			{
				ForDTCanCheckReleased = true;
				//look into this
			}
			if (ForDTCanCheckPressed)
			{
				ForDTCanCheckPressed = false;
				ForDTJustPressed = true;
				GetWorld()->GetTimerManager().SetTimer(
					ForwardDTPressedTimerHandle,
					this,
					&ABaseCharacter::ForwardDTPressed,
					DoubleTapTime,
					false);
			}
			if (ForDTJustPressed && ForDTJustReleased)
			{
				ForwardDoubleTap();
				UE_LOG(LogTemp, Warning, TEXT("Forward Double Tap"));
			}
		}
		else
		{
			IsMovingBack = true;
			AddMovementInput(Direction, Value*BackMulti);
		}
	}
	if (Value == 0.f)
	{
		IsMovingBack = false;
		IsMovingForward = false;
		if (!ForDTJustPressed)
		{
			ForDTCanCheckPressed = true;
		}
		if (ForDTCanCheckReleased && ForDTJustPressed)
		{
			ForDTCanCheckReleased = false;
			ForDTJustReleased = true;
			GetWorld()->GetTimerManager().SetTimer(
					ForwardDTReleasedTimerHandle,
					this,
					&ABaseCharacter::ForwardDTReleased,
					DoubleTapTime,
					false);
		}
	}
}

void ABaseCharacter::ForwardDTPressed()
{
	ForDTJustPressed = false;
}

void ABaseCharacter::ForwardDTReleased()
{
	ForDTJustReleased = false;
}

void ABaseCharacter::ForwardDoubleTap() //Implement ONLY Double Tap WHILE in the Air
{
	TestMovementComponent->Lurch(ControlInputVector);
	ForDTJustPressed = false;
	ForDTJustReleased = false;
}

/* NEW IDEA FOR DOUBLE TAP
 * bool CanCheckPressed
 * bool JustPressed
 * bool CanCheckReleased
 * bool JustReleased
 * 2 timers to flip the values of JustPressed/Released
 * With an input, call the timer and set JustPressed to true (bool to make sure this doesn't happen every input)
 * With NO input, call the timer and set JustReleased to true (bool to make sure this doesn't happen every noninput)
 * With another input, IF JustPressed is true, JustReleased is true, and the same input is done
 * then DoubleTap = true
 *
*/
void ABaseCharacter::MoveLeftRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		IsStrafing = true;
		FVector const Direction = GetActorRightVector();
		AddMovementInput(Direction, Value*StrafeMulti);
		if (Value > 0.f)
		{
			bStrafeRight = true;
			//Right Double Tap
			if (!StrDTJustReleased)
			{
				StrDTCanCheckReleased = true;
			}
			if (StrDTJustPressedL && StrDTJustReleased)
			{
				StrDTJustReleased = false;
			}
			if (StrDTCanCheckPressedR)
			{
				StrDTCanCheckPressedR = false;
				StrDTJustPressedR = true;
				GetWorld()->GetTimerManager().SetTimer(
					StrafeDTPressedRTimerHandle,
					this,
					&ABaseCharacter::StrafeDTPressedR,
					DoubleTapTime,
					false);
			}
			if (StrDTJustPressedR && StrDTJustReleased)
			{
				StrafeDoubleTap();
				UE_LOG(LogTemp, Warning, TEXT("Right Double Tap"));
			}
		}
		if (Value < 0.f)
		{
			bStrafeRight = false;
			//Left Double Tap
			if (!StrDTJustReleased)
			{
				StrDTCanCheckReleased = true;
			}
			if (StrDTJustPressedR && StrDTJustReleased)
			{
				StrDTJustReleased = false;
			}
			if (StrDTCanCheckPressedL)
			{
				StrDTCanCheckPressedL = false;
				StrDTJustPressedL = true;
				GetWorld()->GetTimerManager().SetTimer(
					StrafeDTPressedLTimerHandle,
					this,
					&ABaseCharacter::StrafeDTPressedL,
					DoubleTapTime,
					false);
			}
			if (StrDTJustPressedL && StrDTJustReleased)
			{
				StrafeDoubleTap();
				UE_LOG(LogTemp, Warning, TEXT("Left Double Tap"));
			}
		}
	}
	if (Value == 0.f)
	{
		IsStrafing = false;
		if (!StrDTJustPressedR)
		{
			StrDTCanCheckPressedR = true;
		}
		if (!StrDTJustPressedL)
		{
			StrDTCanCheckPressedL = true;
		}
		if (StrDTCanCheckReleased && (StrDTJustPressedR || StrDTJustPressedL))
		{
			StrDTCanCheckReleased = false;
			StrDTJustReleased = true;
			GetWorld()->GetTimerManager().SetTimer(
				StrafeDTReleasedTimerHandle,
				this,
				&ABaseCharacter::StrafeDTReleased,
				DoubleTapTime,
				false);
		}
	}
}

void ABaseCharacter::StrafeDTPressedR()
{
	StrDTJustPressedR = false;
}

void ABaseCharacter::StrafeDTPressedL()
{
	StrDTJustPressedL = false;
}

void ABaseCharacter::StrafeDTReleased()
{
	StrDTJustReleased = false;
}

void ABaseCharacter::StrafeDoubleTap()
{
	TestMovementComponent->Lurch(ControlInputVector);
	StrDTJustPressedL = false;
	StrDTJustPressedR = false;
	StrDTJustReleased = false;
}


void ABaseCharacter::CustomJump()
{
	TestMovementComponent->StartJump();
}

void ABaseCharacter::SprintWhenAble()
{
	if (!IsSprinting && IsMovingForward)
	{
		TestMovementComponent->Sprint();
		CheckToSprint = false;
		UE_LOG(LogTemp, Warning, TEXT("Sprinting BC now able!"));
	}
}

//TO DO:
// make it so that we sprint as long as W and SHIFT are pressed at same time
// regardless od the order (ie: SHIFT then W, or, W the SHIFT)
// probably need to use a ticking function to check when shift pressed
// once we can sprint, then do it
void ABaseCharacter::Sprint()
{
	if (!IsSprinting)
	{
		//E_LOG(LogTemp, Warning, TEXT("Attempting to Sprint"));
		CheckToSprint = true;
	}
	else if (bSprintDoubleTap)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Attempting to TacSprint"));
		TestMovementComponent->EnterTacSprint();
	}
}

void ABaseCharacter::SprintReleased()
{
	CheckToSprint = false;
	//This can possibly be moved inside of the sprint function
	// OR all 3 can be removed, and we use IE_DoubleTapped input function...
	if (IsSprinting)
	{
		bSprintDoubleTap = true;
		GetWorld()->GetTimerManager().SetTimer(
			SprintDoubleTapTimerHandle,
			this,
			&ABaseCharacter::SprintDoubleTapped,
			DoubleTapTime,
			false);
	}
	
}

void ABaseCharacter::SprintDoubleTapped()
{
	bSprintDoubleTap = false;
}

void ABaseCharacter::CrouchSlideEnter()
{
	if (!IsSliding)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempting to slide!"));
		TestMovementComponent->EnterSlide();
	}
}

void ABaseCharacter::CrouchSlideExit()
{
	if (IsSliding)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempting to NOT slide!"));
		TestMovementComponent->ExitSlide();
	}
}

//
// CAMERA
//

void ABaseCharacter::LookPitch(float Value)
{
	AddControllerPitchInput(Value * AimSensitivity);
}

void ABaseCharacter::LookYaw(float Value)
{
	AddControllerYawInput(Value * AimSensitivity);
}

void ABaseCharacter::TacticalAbility()
{
	if (AbilityComponent == nullptr) return;
	UE_LOG(LogTemp, Warning, TEXT("Character --> Using Tactical Ability!"));
	AbilityComponent->DoGrappleAbility();
}

void ABaseCharacter::SetOverlappingWeapon(AGunWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickUpWidget(false);
	}
	
	OverlappingWeapon = Weapon;
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickUpWidget(true);
	}
		
}

FCollisionQueryParams ABaseCharacter::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;

	TArray<AActor*> CharacterChildren;
	GetAllChildActors(CharacterChildren);
	Params.AddIgnoredActors(CharacterChildren);
	Params.AddIgnoredActor(this);

	return Params;
}








