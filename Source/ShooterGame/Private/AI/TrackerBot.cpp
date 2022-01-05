// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/TrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include <NavigationSystem.h>
#include <Kismet/GameplayStatics.h>
#include "GameFrameWork/Character.h"
#include <NavigationPath.h>
#include <Blueprint/AIBlueprintHelperLibrary.h>
#include <DrawDebugHelpers.h>
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include <ShooterGame\Public\Player\ShooterCharacter.h>
#include "Sound/SoundBase.h"


// Sets default values
ATrackerBot::ATrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ATrackerBot::HandleTakeDamage);

	ShpereComp = CreateDefaultSubobject<USphereComponent>(TEXT("ShpereComp"));
	ShpereComp->SetSphereRadius(200);
	ShpereComp->SetupAttachment(RootComponent);
	ShpereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ShpereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShpereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Set default parameters
	bUseVelocityChange = false;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	// Set explosion defaults
	ExplosionRadius = 200;
	ExplosionDamage = 40;
	SelfDamageInterval = 0.3;
}

// Called when the game starts or when spawned
void ATrackerBot::BeginPlay()
{
	Super::BeginPlay();

	// Find initial point to move to
	NextPathPoint = GetNextPathPoint();

	//UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->GetController(), NextPathPoint);
	
}

void ATrackerBot::HandleTakeDamage(UHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	// Pulse the material on hit
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));

	}
	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);

	}
	
	// Explode when health = 0
	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
	
	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(Health), *GetName());
}

FVector ATrackerBot::GetNextPathPoint()
{
	// Get player location
	ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), MyCharacter);
	
	if (NavPath->PathPoints.Num() > 1)
	{
		// Return next point in the path
		return NavPath->PathPoints[1];
	}

	// Failed to find path
	return GetActorLocation();
}

void ATrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	// Add explosion effect
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	// Apply explosion damage
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoreActors, this, GetInstigatorController(), true);

	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, true, 2.0f, 0, 1.0f);

	// Delete actor immediately
	Destroy();

	// Add sound effect
	UGameplayStatics::SpawnSoundAtLocation(this, ExplodedSound, GetActorLocation());

}

void ATrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

// Called every frame
void ATrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

	if (DistanceToTarget <= RequiredDistanceToTarget)
	{
		NextPathPoint = GetNextPathPoint();

		DrawDebugString(GetWorld(), GetActorLocation(), "Target reached");
	}
	else
	{
		// Keep moving to next target
		FVector ForceDirection = NextPathPoint - GetActorLocation();
		ForceDirection.Normalize();
		ForceDirection *= MovementForce;

		MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), ForceDirection + GetActorLocation(), 32, FColor::Yellow, false, 0.0f, 0, 1);
	}

	DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, true, 2.0f);
}

void ATrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	AShooterCharacter* PlayerCharacter = Cast<AShooterCharacter>(OtherActor);
	if (PlayerCharacter && !bStartedSelfDestruction)
	{
		// Overlapped with a player
		// Start self destruction
		GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ATrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
	
		bStartedSelfDestruction = true;

		// Add sound effect
		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
}

