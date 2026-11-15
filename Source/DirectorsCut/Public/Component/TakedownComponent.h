#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TakedownComponent.generated.h"

class AGuardCharacter;

/* A guard the director could use the clapperboard on right
   now, and whether the takedown would be instant */
struct FTakedownOpportunity
{
	/* Guard in reach, or null when there is none */
	TWeakObjectPtr<AGuardCharacter> Guard;

	/* Whether the takedown knocks the guard out at once. When
	   false the guard sees it coming and it takes time. */
	bool bIsInstant{false};

	/* Whether the director would drop onto the guard from
	   above */
	bool bIsFromAbove{false};
};

/* The clapperboard takedown. Instantly knocks out a guard that
   is stunned, or unaware and facing away, or unaware and below
   the director. A guard who sees it coming can still be cut,
   but it takes a moment during which the director cannot move
   and other guards can shoot him. */
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
	   would be instant */
	FTakedownOpportunity FindTakedownOpportunity() const;

	/* Shows the CUT! popup at a location and slows the game
	   for a moment */
	void PlayCutFeedback(const FVector& CutLocation);

	/* Returns whether a slow takedown is in progress */
	bool IsStruggling() const;

	/* Returns how far the slow takedown has got, from 0 to 1 */
	float GetStruggleProgress() const;

	/* Abandons a slow takedown and frees the director */
	void CancelStruggle();

protected:
	/* Stops the timers and restores normal game speed */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Returns whether the guard is close enough on the ground */
	bool IsGuardInGroundReach(const AGuardCharacter& Guard) const;

	/* Returns whether the guard is below and close enough for a
	   takedown from above */
	bool IsGuardBelowInReach(const AGuardCharacter& Guard) const;

	/* Returns whether the guard cannot see the takedown coming */
	bool IsGuardCaughtOffGuard(const AGuardCharacter& Guard,
		bool bIsFromAbove) const;

	/* Knocks the guard out and shows the CUT! popup */
	void PerformTakedown(const FTakedownOpportunity& Opportunity);

	/* Grabs a guard who sees the director and holds both in
	   place until the takedown lands */
	void BeginStruggle(AGuardCharacter& Guard);

	/* Lands the slow takedown */
	void FinishStruggle();

	/* Lets the director move again after a slow takedown */
	void ReleaseOwnerMovement() const;

	/* Slows the game for a moment to sell the takedown */
	void StartDramaticCut();

	/* Returns the game to normal speed */
	void EndDramaticCut();

	/* Shows a short text popup at a world location */
	void ShowPopup(const FString& Text,
		const FVector& WorldLocation,
		const FLinearColor& Color) const;

	/* Guard held by the slow takedown in progress, if any */
	TWeakObjectPtr<AGuardCharacter> StrugglingGuard;

	/* World time at which the slow takedown started */
	float StruggleStartTimeSeconds;

	/* Timer that lands the slow takedown */
	FTimerHandle StruggleTimerHandle;

	/* Timer that ends the dramatic cut */
	FTimerHandle DramaticCutTimerHandle;
};
