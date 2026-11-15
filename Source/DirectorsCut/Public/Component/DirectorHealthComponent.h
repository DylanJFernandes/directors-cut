#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DirectorHealthComponent.generated.h"

/* Broadcast when the last point of health is lost */
DECLARE_DELEGATE(FHealthDepleted);

/* The director's health: a few hits, with slow regeneration
   once he has been out of combat for a while */
UCLASS()
class UDirectorHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Disables ticking; regeneration runs on a timer */
	UDirectorHealthComponent();

	/* Removes one point of health unless still invulnerable
	   from the previous hit. Returns whether it landed. */
	bool ApplyHit();

	/* Refills health and stops any pending regeneration */
	void RestoreFullHealth();

	/* Pushes regeneration back because combat is ongoing */
	void DelayRegeneration();

	/* Returns the current points of health */
	int32 GetCurrentHealth() const;

	/* Returns the largest possible points of health */
	int32 GetMaximumHealth() const;

	/* Returns seconds since the last hit landed */
	float GetSecondsSinceLastHit() const;

	/* Fires when health reaches zero */
	FHealthDepleted OnHealthDepleted;

protected:
	/* Starts at full health */
	virtual void BeginPlay() override;

private:
	/* Adds one point of health and schedules the next */
	void RegenerateOneHealth();

	/* Returns whether the last hit is too recent for another */
	bool IsInvulnerable() const;

	/* Current points of health */
	UPROPERTY(VisibleAnywhere, Category = "Custom|State")
	int32 CurrentHealth;

	/* World time at which the last hit landed */
	float LastHitTimeSeconds;

	/* Timer that drives regeneration */
	FTimerHandle RegenerationTimerHandle;
};
