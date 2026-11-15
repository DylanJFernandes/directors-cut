#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TommyGunComponent.generated.h"

class AGuardCharacter;
class UBeamMeshComponent;

/* The film reel tommy gun. Once picked up it sprays hitscan
   shots along the camera's aim for as long as the trigger is
   held; each shot takes one point of a guard's health. */
UCLASS()
class UTommyGunComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/* Enables ticking so a held trigger keeps firing */
	UTommyGunComponent();

	/* Gives the owner the gun and shows it */
	void Equip();

	/* Starts firing if the gun is equipped */
	void StartFiring();

	/* Stops firing */
	void StopFiring();

	/* Returns whether the owner has picked the gun up */
	bool IsEquipped() const;

	/* Returns seconds since a shot last hit a guard */
	float GetSecondsSinceLastHit() const;

protected:
	/* Creates the tracer line */
	virtual void BeginPlay() override;

	/* Fires whenever the trigger is held and a shot is due */
	virtual void TickComponent(float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/* Fires one shot along the camera's aim */
	void FireShot();

	/* Returns the first guard the shot line passes close to,
	   and moves the end of the line onto him */
	AGuardCharacter* FindGuardOnShotLine(
		const FVector& ShotStart, FVector& InOutShotEnd) const;

	/* Hides the tracer after its brief flash */
	void HideTracer();

	/* Line drawn from the muzzle along each shot */
	UPROPERTY()
	TObjectPtr<UBeamMeshComponent> Tracer;

	/* Whether the owner has picked the gun up */
	bool bIsEquipped;

	/* Whether the trigger is held */
	bool bIsFiring;

	/* Seconds until the next shot may fire */
	float SecondsUntilNextShot;

	/* World time at which a shot last hit a guard */
	float LastHitTimeSeconds;

	/* Timer that hides the tracer */
	FTimerHandle TracerTimerHandle;
};