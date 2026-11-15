#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MegaphoneComponent.generated.h"

/* The director's megaphone. Throws his voice to the spot he is
   aiming at; guards near that spot walk over to investigate. */
UCLASS()
class UMegaphoneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Disables ticking; the megaphone is fully event driven */
	UMegaphoneComponent();

	/* Makes a noise where the owner is aiming, if recharged.
	   Returns whether a noise was made. */
	bool TryMakeNoise();

	/* Returns how much of the cooldown is left, from 1 (just
	   used) to 0 (ready) */
	float GetCooldownFractionRemaining() const;

	/* Returns where the last noise was made */
	FVector GetLastNoiseLocation() const;

private:
	/* Returns the spot the owner is aiming at, limited by the
	   megaphone's range */
	FVector FindAimedLocation() const;

	/* Sends every guard near the noise to investigate it.
	   Returns how many guards reacted. */
	int32 AlertGuardsNearNoise(const FVector& NoiseLocation) const;

	/* World time at which the megaphone was last used */
	float LastNoiseTimeSeconds;

	/* Where the last noise was made */
	FVector LastNoiseLocation;
};
