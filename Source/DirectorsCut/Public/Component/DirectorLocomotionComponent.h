#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DirectorLocomotionComponent.generated.h"

class ACharacter;

/* The director's fast movement: walking, sprinting and the
   slide. A slide starts by crouching at sprint speed, on the
   ground or on landing from a jump, and sliding off an edge
   gives extra speed. */
UCLASS()
class UDirectorLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Enables ticking to keep speeds and the slide up to date */
	UDirectorLocomotionComponent();

	/* Sets whether the sprint key is held */
	void SetWantsToSprint(bool bInWantsToSprint);

	/* Starts or queues a slide when fast enough, otherwise
	   toggles crouching */
	void HandleCrouchPressed();

	/* Jumps out of a slide keeping its speed. Returns false
	   when the owner is not sliding. */
	bool TrySlideJump();

	/* Starts a queued slide when the owner lands */
	void HandleLanded();

	/* Gives the edge boost when a slide runs off a ledge */
	void HandleWalkedOffLedge();

	/* Ends any slide and forgets a queued one */
	void CancelSlide();

	/* Returns whether the owner is sprinting */
	bool IsSprinting() const;

	/* Returns whether the owner is sliding */
	bool IsSliding() const;

	/* Returns how much of the move input applies right now */
	float GetMoveInputScale() const;

protected:
	/* Caches the owning character and its default friction */
	virtual void BeginPlay() override;

	/* Applies the walk or sprint speed and ends slow slides */
	virtual void TickComponent(float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/* Returns whether the owner is fast enough to slide */
	bool HasSlideEntrySpeed() const;

	/* Starts sliding along the current direction of travel */
	void StartSlide();

	/* Stops sliding and restores normal ground friction */
	void EndSlide(bool bShouldStandUp);

	/* Returns the speed the next slide starts at, lower for
	   each slide chained onto the last */
	float GetNextSlideStartSpeed();

	/* Whether the sprint key is held */
	bool bWantsToSprint;

	/* Whether the owner is sliding */
	bool bIsSliding;

	/* Whether a slide starts as soon as the owner lands */
	bool bIsSlideQueued;

	/* Number of slides chained so far without a rest */
	int32 SlideChainCount;

	/* World time at which the last slide ended */
	float LastSlideEndTimeSeconds;

	/* Ground friction to restore when a slide ends */
	float DefaultGroundFriction;

	/* Braking deceleration to restore when a slide ends */
	float DefaultBrakingDeceleration;

	// Cached Vars
	/* Character that owns this component */
	TWeakObjectPtr<ACharacter> CachedOwnerCharacter;
};
