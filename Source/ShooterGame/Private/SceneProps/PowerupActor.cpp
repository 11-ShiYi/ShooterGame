// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneProps/PowerupActor.h"

// Sets default values
APowerupActor::APowerupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PowerupInterval = 0;
	TotalNumOfTicks = 0;

}

// Called when the game starts or when spawned
void APowerupActor::BeginPlay()
{
	Super::BeginPlay();
}

void APowerupActor::OnTickPowerup()
{
	TicksProcessed++;
	
	OnPowerupTicked();

	if (TotalNumOfTicks <= TicksProcessed)
	{
		OnExpired();

		// Clear timer
		GetWorldTimerManager().ClearTimer(TimerHandle_PoweupTick);
	}
}

void APowerupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APowerupActor::ActivatePowerup()
{
	OnActivated();
	if (PowerupInterval > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PoweupTick, this, &APowerupActor::OnTickPowerup, PowerupInterval, true);
	}
	else
	{
		OnTickPowerup();
	}

}

