#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraRigTurret.generated.h"

class ADirectorCharacter;
class AGuardCharacter;
class UBeamMeshComponent;
class UStaticMeshComponent;

/* A studio camera rig the director can climb onto during the
   upload. While mounted he cannot move, but its spotlight stuns
   the guard he aims at from far away, again and again. */
UCLASS()
class ACameraRigTurret : public AActor
{
	GENERATED_BODY()

public:
	/* Creates the rig's base, head and spotlight beam */
	ACameraRigTurret();

	/* Returns whether E would mount the rig from there */
	bool CanBeMountedFrom(const FVector& UserLocation) const;

	/* Returns where the director stands while mounted */
	FVector GetSeatLocation() const;

	/* Fires the spotlight along the view direction, stunning
	   the guard closest to the aim. Returns whether it fired. */
	bool TryFireSpotlight(const FVector& ViewLocation,
		const FVector& ViewDirection);

protected:
	/* Applies meshes and colors */
	virtual void BeginPlay() override;

private:
	/* Returns the guard closest to the aim line, if any */
	AGuardCharacter* FindAimedGuard(const FVector& ViewLocation,
		const FVector& ViewDirection) const;

	/* Returns where the spotlight beam starts */
	FVector GetLensLocation() const;

	/* Hides the spotlight beam after its flash */
	void HideSpotlightBeam();

	/* Tripod base of the rig */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/* Camera head of the rig */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	/* Beam shown for an instant when the spotlight fires */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UBeamMeshComponent> SpotlightBeam;

	/* World time at which the spotlight last fired */
	float LastFireTimeSeconds;

	/* Timer that hides the spotlight beam */
	FTimerHandle BeamHideTimerHandle;
};
