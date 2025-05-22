// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAbilitiesComponent.h"

#include "BaseCharacter.h"
#include "Camera/CameraComponent.h"
#include "SolarPunkShooterTest/Movement/TestMovementComponent.h"

// Sets default values for this component's properties
UCharacterAbilitiesComponent::UCharacterAbilitiesComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCharacterAbilitiesComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacterOwner = Cast<ABaseCharacter>(GetOwner());
	if (BaseCharacterOwner != nullptr) { BaseCharacterMovement = BaseCharacterOwner->GetBaseCharacterMovement(); }

	// Initialize Variables
	bGrappleCooldownFinished = true;
	
}



// Called every frame
void UCharacterAbilitiesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCharacterAbilitiesComponent::DoGrappleAbility()
{
	if (BaseCharacterMovement->IsGrappling)
	{
		BaseCharacterMovement->ExitGrapple();
		return;
	}
	if (!bGrappleCooldownFinished) { return; }
	UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Ability Started!"));
	if (BaseCharacterOwner == nullptr || BaseCharacterMovement == nullptr) { return; }
	UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Passed Condition 1"));
	if (BaseCharacterMovement->IsGrappling || BaseCharacterMovement->IsMantling) { return; }
	UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Passed Condition 2"));
	FVector Start = BaseCharacterOwner->Camera->GetComponentLocation();
	FVector End = Start + BaseCharacterOwner->Camera->GetComponentRotation().Vector() * GrappleReachDistance;
	auto Params = BaseCharacterOwner->GetIgnoreCharacterParams();
	FHitResult GrappleHit;
	GetWorld()->LineTraceSingleByProfile(GrappleHit, Start, End, "BlockAll", Params);

	if (!GrappleHit.IsValidBlockingHit())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Grapple Did Not Connect!"));
	}
	else
	{
		BaseCharacterMovement->GrappleConnectPoint = GrappleHit.Location;
		BaseCharacterMovement->GrappleCharStartPoint = BaseCharacterOwner->Camera->GetComponentLocation();
		BaseCharacterMovement->OriginalGrappleDirection = BaseCharacterOwner->Camera->GetComponentRotation().Vector().GetSafeNormal();
		BaseCharacterMovement->EnterGrapple();
		UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Starting Grapple!"));

		bGrappleCooldownFinished = false;
	}
}

void UCharacterAbilitiesComponent::StartGrappleCooldown()
{
	UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Starting Cooldown!"));
	GetWorld()->GetTimerManager().SetTimer(
		GrappleCooldownTimerHandle,
		this,
		&UCharacterAbilitiesComponent::GrappleCooldownFinished,
		GrappleCooldownTime,
		false);
}


void UCharacterAbilitiesComponent::GrappleCooldownFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("Ability Component --> Grapple: Cooldown Finished!"));
	bGrappleCooldownFinished = true;
}

