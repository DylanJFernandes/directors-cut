#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MegaphoneComponent.generated.h"

/* The director's megaphone. He shouts through it where he
   stands; every guard within earshot walks over to look, which
   pulls them off their routes while he moves on. */
UCLASS()
class UMegaphoneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Disables ticking; the megaphone is fully event driven */
	UMegaphoneComponent();

	/* Shouts from where the owner stands, if recharged.
	   Returns whether a noise was made. */
	bool TryMakeNoise();

	/* Returns how much of the cooldown is left, from 1 (just
	   used) to 0 (ready) */
	float GetCooldownFractionRemaining() const;

	/* Returns where the last noise was made */
	FVector GetLastNoiseLocation() const;

	/* Returns seconds since the megaphone was last used */
	float GetSecondsSinceLastNoise() const;

private:
	/* Sends every guard near the noise to investigate it.
	   Returns how many guards reacted. */
	int32 AlertGuardsNearNoise(const FVector& NoiseLocation) const;

	/* World time at which the megaphone was last used */
	float LastNoiseTimeSeconds;

	/* Where the last noise was made */
	FVector LastNoiseLocation;
};