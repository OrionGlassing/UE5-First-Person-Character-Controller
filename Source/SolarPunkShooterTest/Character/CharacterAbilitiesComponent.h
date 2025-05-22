// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterAbilitiesComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SOLARPUNKSHOOTERTEST_API UCharacterAbilitiesComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterAbilitiesComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class ABaseCharacter* BaseCharacterOwner;
	class UTestMovementComponent* BaseCharacterMovement;
	
	//
	//Grapple
	//

	//Customizable
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleReachDistance;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleDisconnectDistance;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleMinDistance;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrapplePullAcceleration;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleMoveMaxAcceleration;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleMinSpeed;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleMaxSpeed;
	UPROPERTY(EditDefaultsOnly, Category = Grapple) float GrappleCooldownTime;
	//Variables
	bool bGrappleCooldownFinished;
	FTimerHandle GrappleCooldownTimerHandle;
	//Functions
	void DoGrappleAbility();
	void StartGrappleCooldown();
	void GrappleCooldownFinished();
	
};
