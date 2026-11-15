#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraFlashComponent.generated.h"

class AGuardCharacter;

/* The director's camera flash. Stuns every guard inside a cone
   in front of the camera, then needs time to recharge. */
UCLASS()
class UCameraFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Disables ticking; the flash is fully event driven */
	UCameraFlashComponent();

	/* Fires the flash if it has recharged. Returns whether it
	   fired. */
	bool TryFlash();

	/* Returns how much of the cooldown is left, from 1 (just
	   fired) to 0 (ready) */
	float GetCooldownFractionRemaining() const;

	/* Returns seconds since the flash last fired */
	float GetSecondsSinceLastFlash() const;

private:
	/* Stuns every guard caught in the flash cone. Returns how
	   many were stunned. */
	int32 StunGuardsInCone() const;

	/* Returns whether the guard is inside the flash cone and
	   not behind a wall */
	bool IsGuardCaughtInFlash(const AGuardCharacter& Guard,
		const FVector& FlashOrigin,
		const FVector& FlashDirection) const;

	/* World time at which the flash last fired */
	float LastFlashTimeSeconds;
};
