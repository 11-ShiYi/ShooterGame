// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterCharacter.generated.h"


class UCameraComponent;
class USpringArmComponent;
class UHealthComponent;
class AWeapon;


UCLASS()
class SHOOTERGAME_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShooterCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bIsCrouch;

	float DefaultCapsuleHeight;
	
	float CrouchCapsuleHeight;

	UFUNCTION(BlueprintCallable)
	void BeginCrouch();

	UFUNCTION(BlueprintCallable)
	void EndCrouch();

	void CrouchImpl(float DeltaTime);

	void BeignJump();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	bool bWantsToZoom;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
		float ZoomedFOV; // The value after zoomed

	float DefaultFOV; // Default FOV set during begin play

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0, ClampMax = 100))
		float ZoomInterpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0, ClampMax = 10))
		float CrouchInterSpeed;

	void BeginZoom();

	void EndZoom();

	UPROPERTY(Replicated)
	AWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<AWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

	void StartFire();

	void StopFire();

	UFUNCTION()
		void OnHealthChanged(UHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// Pawn died previously
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bDied;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Overwrite GetPawnViewLocation()
	virtual FVector GetPawnViewLocation() const override;

};
