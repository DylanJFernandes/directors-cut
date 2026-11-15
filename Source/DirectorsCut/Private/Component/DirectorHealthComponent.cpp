#include "Component/DirectorHealthComponent.h"

#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "TimerManager.h"

UDirectorHealthComponent::UDirectorHealthComponent()
	: CurrentHealth{0},
	  LastHitTimeSeconds{-1000.0f}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDirectorHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	RestoreFullHealth();
}

bool UDirectorHealthComponent::ApplyHit()
{
	if (CurrentHealth <= 0 || IsInvulnerable())
	{
		return false;
	}
	--CurrentHealth;
	LastHitTimeSeconds = GetWorld()->GetTimeSeconds();
	if (CurrentHealth <= 0)
	{
		OnHealthDepleted.ExecuteIfBound();
		return true;
	}
	DelayRegeneration();
	return true;
}

bool UDirectorHealthComponent::IsInvulnerable() const
{
	return GetSecondsSinceLastHit()
		< UBacklotTuningSettings::Get().HitInvulnerabilitySeconds;
}

void UDirectorHealthComponent::RestoreFullHealth()
{
	CurrentHealth = GetMaximumHealth();
	GetWorld()->GetTimerManager().ClearTimer(
		RegenerationTimerHandle);
}

void UDirectorHealthComponent::DelayRegeneration()
{
	if (CurrentHealth >= GetMaximumHealth())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		RegenerationTimerHandle, this,
		&UDirectorHealthComponent::RegenerateOneHealth,
		UBacklotTuningSettings::Get()
			.HealthRegenerationDelaySeconds);
}

void UDirectorHealthComponent::RegenerateOneHealth()
{
	if (CurrentHealth <= 0
		|| CurrentHealth >= GetMaximumHealth())
	{
		return;
	}
	++CurrentHealth;
	if (CurrentHealth < GetMaximumHealth())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RegenerationTimerHandle, this,
			&UDirectorHealthComponent::RegenerateOneHealth,
			UBacklotTuningSettings::Get()
				.HealthRegenerationIntervalSeconds);
	}
}

int32 UDirectorHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

int32 UDirectorHealthComponent::GetMaximumHealth() const
{
	return UBacklotTuningSettings::Get().PlayerMaximumHealth;
}

float UDirectorHealthComponent::GetSecondsSinceLastHit() const
{
	return GetWorld()->GetTimeSeconds() - LastHitTimeSeconds;
}
