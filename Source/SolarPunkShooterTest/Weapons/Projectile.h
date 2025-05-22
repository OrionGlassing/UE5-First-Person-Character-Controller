// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class SOLARPUNKSHOOTERTEST_API AProjectile : public AActor
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	AProjectile();
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Damage
	UPROPERTY(EditAnywhere)
	float Damage;

	// Settings
	UPROPERTY(EditAnywhere) bool RotateWithVelocity;
	UPROPERTY(EditAnywhere) float cFriction;
	
	// Bouncing
	UPROPERTY(EditAnywhere) bool ShouldBounce;
	UPROPERTY(EditAnywhere) float cBounciness;
	UPROPERTY(EditAnywhere) int32 MaxBounces;

	int32 NumBounces;

	// Aim assist
	UPROPERTY(EditAnywhere) bool HasAimAssist;
	UPROPERTY(EditAnywhere) float AimAssistStrength;
	
	
	
private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;
	
};
