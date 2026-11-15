#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleComponent.generated.h"

class ACharacter;
class AGrappleHookPoint;

/* The film reel grapple. Every frame it picks the hook point
   closest to the center of the screen, and on request it flies
   the owning character to that point along a smooth arc. */
UCLASS()
class UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Enables ticking for targeting and arc travel */
	UGrappleComponent();

	/* Starts flying to the targeted hook point. Returns false
	   when no hook point is targeted. */
	bool TryStartGrapple();

	/* Stops an active grapple and lets the owner fall */
	void CancelGrapple();

	/* Returns the hook point E would grapple to, if any */
	AGrappleHookPoint* GetTargetedHookPoint() const;

	/* Returns whether the owner is flying along a grapple */
	bool IsGrappling() const;

	/* Returns where the active grapple is heading */
	FVector GetGrappleDestination() const;

protected:
	/* Caches the owning character */
	virtual void BeginPlay() override;

	/* Updates the target and advances an active grapple */
	virtual void TickComponent(float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/* Retargets and updates the look of the hook points */
	void UpdateTargetedHookPoint();

	/* Returns the reachable hook point nearest screen center */
	AGrappleHookPoint* FindBestHookPoint() const;

	/* Returns the angle in degrees between the view direction
	   and the hook point, or a negative value when the hook
	   point cannot be grappled */
	float GetAimAngleToHookPoint(
		const AGrappleHookPoint& HookPoint,
		const FVector& ViewLocation,
		const FVector& ViewDirection) const;

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

	// Cached Vars
	/* Character that owns this component */
	TWeakObjectPtr<ACharacter> CachedOwnerCharacter;

	/* Hook point E would grapple to */
	TWeakObjectPtr<AGrappleHookPoint> TargetedHookPoint;

	/* Hook point the active grapple is flying to */
	TWeakObjectPtr<AGrappleHookPoint> DestinationHookPoint;
};
