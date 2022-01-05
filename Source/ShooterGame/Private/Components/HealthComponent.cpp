// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"
#include <Runtime\Engine\Public\Net\UnrealNetwork.h>

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	DefaultHealth = 100;

	// Set health replicated
	SetIsReplicated(true);
}


//Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only hook the component to the owning actor if we are server
	if (GetOwnerRole() == ROLE_Authority)
	{
		AActor* MyActor = GetOwner();
		if (MyActor)
		{
			MyActor->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
		}
	}
	
	
	Health = DefaultHealth;
}


void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return;
	}

	// Update health clamped
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));

	// After update health, call the macro and broadcast
	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// When using HealthRegeneration and PainGeneration props
void UHealthComponent::HealthChange(float HealthAmout)
{ 
	if (HealthAmout == 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealthAmout, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s (+%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealthAmout));

	OnHealthChanged.Broadcast(this, Health, -HealthAmout, nullptr, nullptr, nullptr);

}

// Set replicate rule
void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
}