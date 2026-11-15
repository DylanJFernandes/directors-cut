#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TakedownComponent.generated.h"

class AGuardCharacter;

/* A guard the director could use the clapperboard on right
   now, and whether the attempt would work */
struct FTakedownOpportunity
{
	/* Guard in reach, or null when there is none */
	TWeakObjectPtr<AGuardCharacter> Guard;

	/* Whether the takedown would knock the guard out */
	bool bWillSucceed{false};

	/* Whether the director would drop onto the guard from
	   above */
	bool bIsFromAbove{false};
};

/* The clapperboard takedown. Knocks out a guard that is
   stunned, or unaware and facing away, or below the director.
   Trying it on an alert guard throws the director back. */
UCLASS()
class UTakedownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Disables ticking; takedowns are fully event driven */
	UTakedownComponent();

	/* Attempts a takedown on the guard in reach, if any */
	void TryTakedown();

	/* Returns the guard in reach and whether a takedown on it
	   would succeed */
	FTakedownOpportunity FindTakedownOpportunity() const;

private:
	/* Returns whether the guard is close enough on the ground */
	bool IsGuardInGroundReach(const AGuardCharacter& Guard) const;

	/* Returns whether the guard is below and close enough for a
	   takedown from above */
	bool IsGuardBelowInReach(const AGuardCharacter& Guard) const;

	/* Returns whether a takedown on this guard would succeed */
	bool WouldTakedownSucceed(const AGuardCharacter& Guard,
		bool bIsFromAbove) const;

	/* Knocks the guard out and shows the CUT! popup */
	void PerformTakedown(const FTakedownOpportunity& Opportunity);

	/* Alerts the guard and throws the director back */
	void FailTakedown(AGuardCharacter& Guard);

	/* Shows a short text popup at a world location */
	void ShowPopup(const FString& Text,
		const FVector& WorldLocation,
		const FLinearColor& Color) const;
};
