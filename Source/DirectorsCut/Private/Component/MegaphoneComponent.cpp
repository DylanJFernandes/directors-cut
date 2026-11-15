#include "Component/MegaphoneComponent.h"

#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

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
	LastNoiseLocation = FindAimedLocation();
	AlertGuardsNearNoise(LastNoiseLocation);
	return true;
}

FVector UMegaphoneComponent::FindAimedLocation() const
{
	const APawn* OwnerPawn{Cast<APawn>(GetOwner())};
	if (!OwnerPawn || !OwnerPawn->GetController())
	{
		return GetOwner()->GetActorLocation();
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation,
		ViewRotation);
	const FVector RangeEnd{ViewLocation + ViewRotation.Vector()
		* UBacklotTuningSettings::Get().MegaphoneRange};

	FCollisionQueryParams QueryParams{
		TEXT("MegaphoneAim"), false, GetOwner()};
	FHitResult Hit;
	const bool bHitSomething{
		GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation,
			RangeEnd, ECC_Visibility, QueryParams)};
	return bHitSomething ? Hit.ImpactPoint : RangeEnd;
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
