#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleComponent.generated.h"

class ACharacter;
class AGrappleHookPoint;
class AGuardCharacter;

/* Broadcast when a grapple aimed at a guard reaches him */
DECLARE_DELEGATE_OneParam(FGrappleReachedGuard, AGuardCharacter&);

/* The film reel grapple. Every frame it picks the guard or hook
   point closest to the center of the screen, and on request it
   flies the owning character there along a smooth arc. Zipping
   to a guard keeps the speed of the flight. */
UCLASS()
class UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Enables ticking for targeting and arc travel */
	UGrappleComponent();

	/* Starts flying to the targeted guard or hook point.
	   Returns false when there is no target or the film reel
	   is still on cooldown. */
	bool TryStartGrapple();

	/* Stops an active grapple and lets the owner fall */
	void CancelGrapple();

	/* Returns the hook point E would grapple to, if any */
	AGrappleHookPoint* GetTargetedHookPoint() const;

	/* Returns the guard E would zip to, if any */
	AGuardCharacter* GetTargetedGuard() const;

	/* Returns whether the owner is flying along a grapple */
	bool IsGrappling() const;

	/* Returns where the active grapple is heading */
	FVector GetGrappleDestination() const;

	/* Returns how much of the cooldown is left, from 1 (just
	   used) to 0 (ready) */
	float GetCooldownFractionRemaining() const;

	/* Fires when a grapple aimed at a guard reaches him */
	FGrappleReachedGuard OnGrappleReachedGuard;

protected:
	/* Caches the owning character */
	virtual void BeginPlay() override;

	/* Updates the target and advances an active grapple */
	virtual void TickComponent(float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/* Retargets and updates the look of the hook points */
	void UpdateTargets();

	/* Returns the reachable hook point nearest screen center */
	AGrappleHookPoint* FindBestHookPoint(
		const FVector& ViewLocation,
		const FVector& ViewDirection) const;

	/* Returns the reachable guard nearest screen center */
	AGuardCharacter* FindBestGuard(const FVector& ViewLocation,
		const FVector& ViewDirection) const;

	/* Returns the angle in degrees between the view direction
	   and the hook point, or a negative value when the hook
	   point cannot be grappled */
	float GetAimAngleToHookPoint(
		const AGrappleHookPoint& HookPoint,
		const FVector& ViewLocation,
		const FVector& ViewDirection) const;

	/* Returns the angle in degrees between the view direction
	   and a location */
	float GetAimAngleToLocation(const FVector& Location,
		const FVector& ViewLocation,
		const FVector& ViewDirection) const;

	/* Returns whether a location is neither too close nor
	   farther than the given range from the owner */
	bool IsWithinGrappleDistance(const FVector& Location,
		float MaximumDistance) const;

	/* Returns whether nothing solid blocks the view of a point */
	bool HasLineOfSight(const FVector& ViewLocation,
		const FVector& TargetLocation) const;

	/* Moves the owner one step along the arc */
	void AdvanceGrapple(float DeltaTime);

	/* Returns the point on the arc at the given progress */
	FVector GetArcLocation(float Progress) const;

	/* Ends the grapple and hands the owner back to falling
	   with some leftover momentum */
	void FinishGrapple();

	/* Whether the owner is flying along a grapple */
	bool bIsGrappling;

	/* Where the active grapple started */
	FVector GrappleStartLocation;

	/* Where the active grapple is heading */
	FVector GrappleDestination;

	/* Seconds the active grapple takes in total */
	float GrappleDurationSeconds;

	/* Seconds the active grapple has been running */
	float GrappleElapsedSeconds;

	/* World time at which the last grapple started */
	float LastGrappleStartTimeSeconds;

	// Cached Vars
	/* Character that owns this component */
	TWeakObjectPtr<ACharacter> CachedOwnerCharacter;

	/* Hook point E would grapple to */
	TWeakObjectPtr<AGrappleHookPoint> TargetedHookPoint;

	/* Guard E would zip to */
	TWeakObjectPtr<AGuardCharacter> TargetedGuard;

	/* Hook point the active grapple is flying to */
	TWeakObjectPtr<AGrappleHookPoint> DestinationHookPoint;

	/* Guard the active grapple is flying to */
	TWeakObjectPtr<AGuardCharacter> DestinationGuard;
};
