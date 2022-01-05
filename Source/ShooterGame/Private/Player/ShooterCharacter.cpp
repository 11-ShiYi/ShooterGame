// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFrameWork/SpringArmComponent.h"
#include "GameFrameWork/PawnMovementComponent.h"
#include "GameFrameWork/Character.h"
#include "WeaponSystem/Weapon.h"
#include <ShooterGame\ShooterGame.h>
#include "Components/CapsuleComponent.h"
#include "ShooterGame/Public/Components/HealthComponent.h"
#include <GameFramework/Actor.h>
#include "Net/UnrealNetwork.h"


// Sets default values
AShooterCharacter::AShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true; //Rotate camera based on character's view.

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	// Create health comp
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	
//		//Enable crouch --Default function, now use custom version
// 		GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// Enable jump
	GetMovementComponent()->GetNavAgentPropertiesRef().bCanJump = true;

	// Set Capsule ignore weapons
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	
	// Set default Zoom parameters
	ZoomedFOV = 60.0f;
	ZoomInterpSpeed = 15.0f;

	// set weapon socket name
	WeaponAttachSocketName = "WeaponSocket";


	//Get default capsule height when stand up
	DefaultCapsuleHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	//Set CrouchSpeed
	CrouchInterSpeed = 3;
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Get default FOV
	DefaultFOV = CameraComp->FieldOfView;

	// Call OnHealthChanged
	HealthComp->OnHealthChanged.AddDynamic(this, &AShooterCharacter::OnHealthChanged);

	if (GetLocalRole() == ROLE_Authority) // Spawn default weapons in server
	{
		// Spawn a default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<AWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
		}

	}
	
}

//Add move forward and backward
void AShooterCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector()*Value);
}

//Add move left and right
void AShooterCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void AShooterCharacter::BeginCrouch()
{
	bIsCrouch = true;
	
	// When crouch, set the height of capsule collision smaller
	CrouchCapsuleHeight = 44;
	

}

void AShooterCharacter::EndCrouch()
{
	//UnCrouch();
	bIsCrouch = false;
}

void AShooterCharacter::CrouchImpl(float DeltaTime)
{
	float TargetHeight = bIsCrouch ? CrouchCapsuleHeight : DefaultCapsuleHeight;

	// Smooth the height of Capsule
	float NewCapsuleHeight = FMath::FInterpTo(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), TargetHeight, DeltaTime, CrouchInterSpeed);
	GetCapsuleComponent()->SetCapsuleHalfHeight(NewCapsuleHeight);

	// When the height of capsule changes, it will stand float, not get to the floor because the relative location doesn't change. 
	// So calculate the distance between the lower end of capsule and the floor
	float Dist = TargetHeight - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	float DeltaMovCaps = Dist * DeltaTime * CrouchInterSpeed;

	GetCapsuleComponent()->SetRelativeLocation(FVector(GetCapsuleComponent()->GetRelativeLocation().X, GetCapsuleComponent()->GetRelativeLocation().Y, (GetCapsuleComponent()->GetRelativeLocation().Z + DeltaMovCaps)));
	
	// CharacterMesh is attached to the capsule, when capsule moves, the mesh moves too, complement it.
	GetMesh()->SetRelativeLocation(FVector(GetMesh()->GetRelativeLocation().X, GetMesh()->GetRelativeLocation().Y, (GetMesh()->GetRelativeLocation().Z - DeltaMovCaps)));

}

void AShooterCharacter::BeignJump()
{
	Jump();
}


void AShooterCharacter::BeginZoom()
{
	bWantsToZoom = true;
}

void AShooterCharacter::EndZoom()
{
	bWantsToZoom = false;

}

void AShooterCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AShooterCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AShooterCharacter::OnHealthChanged(UHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{
		// Die
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		DetachFromControllerPendingDestroy();
		
		SetLifeSpan(5.0f);
	}
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Zoom when aim down the sight
	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;

	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
	CameraComp->SetFieldOfView(NewFOV);

	// Call Crouch()
	CrouchImpl(DeltaTime);
	
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	
	//&AShooterCharacter::AddControllerPitchInput is available.
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::AddControllerYawInput);

	//Bind Crouch to input
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AShooterCharacter::EndCrouch);

	//Bind Jump to input
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);

	//Bind Zoom to input
	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &AShooterCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &AShooterCharacter::EndZoom);

	//Bind Fire to input
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::StartFire); 
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::StopFire); 
}

FVector AShooterCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

// Set replicate rule
void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
	DOREPLIFETIME(AShooterCharacter, bDied);
}