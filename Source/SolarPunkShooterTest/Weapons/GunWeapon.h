// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GunWeapon.generated.h"

UENUM()
enum class EWeaponState : uint8
{
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_Max UMETA(DisplayName = "DefaultMAX")
};

UENUM()
enum class EWeaponType : uint8
{
	EWT_BoltActionSniper UMETA(DisplayName = "BoltActionSniper"),
	EWT_MagSniper		 UMETA(DisplayName = "MagSniper"),
	EWT_Deagle			 UMETA(DisplayName = "Deagle"),
	EWT_Revolver		 UMETA(DisplayName = "Revolver"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class SOLARPUNKSHOOTERTEST_API AGunWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGunWeapon();
	virtual void Tick(float DeltaTime) override;

	//Functions

	void ShowPickUpWidget(bool bShowWidget);

	void Dropped();
	void SetWeaponState(EWeaponState State);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
		);

	//Functions

	

private:
	UPROPERTY(VisibleAnywhere, Category = Components)
	UStaticMeshComponent* TestMesh;

	UPROPERTY(VisibleAnywhere, Category = Components)
	class USphereComponent* AreaSphere;
	
	//UPROPERTY(VisibleAnywhere, Category = Components)
	//class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(VisibleAnywhere, Category = Properties)
	EWeaponState WeaponState;
	
	UPROPERTY(EditAnywhere, Category = Properties)
	int32 AmmoCount;

	UPROPERTY(EditAnywhere, Category = Properties)
	int32 MagazineCount;

	UPROPERTY()
	class ABaseCharacter* BaseCharacterOwner;

	
	
};
