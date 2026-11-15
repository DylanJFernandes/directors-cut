#include "Component/MegaphoneComponent.h"

#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/BacklotSoundLibrary.h"

UMegaphoneComponent::UMegaphoneComponent()
	: LastNoiseTimeSeconds{-1000.0f},
	  LastNoiseLocation{FVector::ZeroVector}
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMegaphoneComponent::TryMakeNoise()
{
	if (GetCooldownFractionRemaining() > 0.0f)
	{
		return false;
	}
	LastNoiseTimeSeconds = GetWorld()->GetTimeSeconds();
	LastNoiseLocation = GetOwner()->GetActorLocation();
	BacklotSoundLibrary::PlayMegaphone(this, LastNoiseLocation);
	AlertGuardsNearNoise(LastNoiseLocation);
	return true;
}

int32 UMegaphoneComponent::AlertGuardsNearNoise(
	const FVector& NoiseLocation) const
{
	const float NoiseRadius{
		UBacklotTuningSettings::Get().MegaphoneNoiseRadius};
	int32 ReactingGuardCount{0};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		if (FVector::Dist(GuardIterator->GetActorLocation(),
			NoiseLocation) <= NoiseRadius)
		{
			GuardIterator->InvestigateLocation(NoiseLocation);
			++ReactingGuardCount;
		}
	}
	return ReactingGuardCount;
}

float UMegaphoneComponent::GetCooldownFractionRemaining() const
{
	const float SecondsSinceNoise{static_cast<float>(
		GetWorld()->GetTimeSeconds() - LastNoiseTimeSeconds)};
	return FMath::Clamp(1.0f - SecondsSinceNoise
		/ UBacklotTuningSettings::Get().MegaphoneCooldownSeconds,
		0.0f, 1.0f);
}

FVector UMegaphoneComponent::GetLastNoiseLocation() const
{
	return LastNoiseLocation;
}

float UMegaphoneComponent::GetSecondsSinceLastNoise() const
{
	return static_cast<float>(
		GetWorld()->GetTimeSeconds() - LastNoiseTimeSeconds);
}
