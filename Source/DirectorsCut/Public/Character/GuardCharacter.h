#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GuardCharacter.generated.h"

class AAIController;
class ADirectorCharacter;
class UBeamMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/* What a studio guard is currently doing */
UENUM()
enum class EGuardState : uint8
{
	Patrol,
	Suspicious,
	Alert,
	Search,
	Stunned,
	TakenDown
};

/* An armed studio guard. Patrols between points, fills a
   detection meter while it sees or hears the director, then
   investigates, chases and shoots with a telegraphed laser. */
UCLASS()
class AGuardCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/* Creates the guard's mesh, state marker and laser */
	AGuardCharacter();

	/* Makes this guard return to hunting the player instead of
	   giving up, as the upload wave guards do */
	void MakeRelentless();

	/* Called by the AI controller when sight of the player is
	   gained or lost */
	void HandlePlayerSightChanged(bool bInCanSeePlayer);

	/* Freezes the guard for the given time */
	void ApplyStun(float StunSeconds);

	/* Knocks the guard out for good */
	void ReceiveTakedown();

	/* Takes one tommy gun hit, which alerts the guard to the
	   shooter. Returns whether the hit knocked him out. */
	bool ReceiveBulletHit(const FVector& ShooterLocation,
		const FVector& ShotDirection);

	/* Puts the guard straight into chasing and shooting */
	void BecomeAlert(const FVector& KnownPlayerLocation);

	/* Sends a guard that is not alert to look at a location */
	void InvestigateLocation(const FVector& Location);

	/* Returns the guard to its first patrol point, calm */
	void ResetToPatrolStart();

	/* Returns what the guard is currently doing */
	EGuardState GetGuardState() const;

	/* Returns the detection meter, from 0 to 1 */
	float GetDetectionMeter() const;

	/* Returns whether the guard is stunned */
	bool IsStunned() const;

	/* Returns whether the guard is chasing and shooting */
	bool IsAlert() const;

	/* Returns whether the guard has been knocked out */
	bool IsTakenDown() const;

	/* Returns whether the guard is facing the given location */
	bool IsFacingLocation(const FVector& Location) const;

protected:
	/* Applies meshes, colors and tuning values */
	virtual void BeginPlay() override;

	/* Runs detection and the current state every frame */
	virtual void Tick(float DeltaSeconds) override;

private:
	/* Turns the placed patrol points into world locations,
	   starting from where the guard stands */
	void BuildPatrolLocations();

	// Detection
	/* Fills or drains the detection meter and reacts to it */
	void UpdateDetectionMeter(float DeltaSeconds);

	/* Turns suspicious or alert once the meter is high enough */
	void ReactToDetectionMeter();

	/* Returns meter gained per second from seeing the player */
	float GetSightDetectionRate(
		const ADirectorCharacter& Player) const;

	/* Returns meter gained per second from hearing the player */
	float GetHearingDetectionRate(
		const ADirectorCharacter& Player) const;

	// States
	/* Switches state and updates speed, focus and marker */
	void EnterState(EGuardState NewState);

	/* Walks the patrol route, waiting at each point */
	void UpdatePatrol(float DeltaSeconds);

	/* Walks to the last known location and looks around */
	void UpdateSuspicious(float DeltaSeconds);

	/* Chases the player and shoots when in range */
	void UpdateAlert(float DeltaSeconds);

	/* Looks for the player after losing them */
	void UpdateSearch(float DeltaSeconds);

	/* Counts the stun down and recovers */
	void UpdateStunned(float DeltaSeconds);

	/* Walks to the last known location, then turns on the
	   spot. Returns seconds spent looking around so far. */
	float InvestigateLastKnownLocation(float DeltaSeconds);

	/* Warns nearby guards about the player */
	void ShoutToNearbyGuards() const;

	// Movement
	/* Moves toward a location, pathfinding when possible */
	void MoveTowardLocation(const FVector& GoalLocation);

	/* Returns whether a new path request is due for the goal */
	bool ShouldRequestNewPath(const FVector& GoalLocation) const;

	/* Asks the AI controller for a path to the goal */
	void RequestPathTo(const FVector& GoalLocation);

	/* Stops walking and forgets the current goal */
	void StopMoving();

	/* Faces the player when alert, else the walk direction */
	void ApplyFacingModeForState();

	/* Returns whether the guard is standing at a location */
	bool HasReachedLocation(const FVector& Location) const;

	/* Sets the walking speed for the current state */
	void ApplySpeedForState();

	// Shooting
	/* Starts aiming the laser at the player */
	void BeginShotWindup();

	/* Tracks, then locks, the aim and fires when wound up */
	void UpdateShotWindup(float DeltaSeconds);

	/* Fires along the locked aim line */
	void FireShot();

	/* Cancels any shot in progress and hides the laser */
	void CancelShot();

	/* Hides the laser after the firing flash */
	void HideLaser();

	/* Returns where the guard's shots start */
	FVector GetMuzzleLocation() const;

	/* Returns where the shot line ends: the first wall hit or
	   the end of the weapon's range */
	FVector GetShotEndLocation() const;

	// Presentation
	/* Colors the marker above the guard's head for its state */
	void UpdateStateMarker();

	/* Draws the vision cone when debug drawing is on */
	void DrawDebugVisionCone() const;

	/* Returns the player character, caching it on first use */
	ADirectorCharacter* GetPlayer();

	/* Returns this guard's AI controller */
	AAIController* GetGuardController() const;

	/* Marker above the head, colored by state */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> StateMarker;

	/* Laser line that telegraphs each shot */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UBeamMeshComponent> LaserBeam;

	/* Greybox gun held in front of the guard */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> GunMesh;

	/* Material instance that carries the marker color */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> StateMarkerMaterial;

	// State
	/* What the guard is currently doing */
	UPROPERTY(VisibleAnywhere, Category = "Custom|State")
	EGuardState CurrentState;

	/* State to return to when a stun ends */
	EGuardState StateBeforeStun;

	/* Detection meter, from 0 to 1 */
	UPROPERTY(VisibleAnywhere, Category = "Custom|State")
	float DetectionMeter;

	/* Tommy gun hits the guard can still take */
	UPROPERTY(VisibleAnywhere, Category = "Custom|State")
	int32 HealthRemaining;

	/* Whether AI perception currently sees the player */
	bool bCanSeePlayer;

	/* Whether this guard never gives up the hunt */
	bool bIsRelentless;

	/* Where the player was last seen or heard */
	FVector LastKnownPlayerLocation;

	/* Seconds spent in the current look-around or wait */
	float StateElapsedSeconds;

	/* Seconds since the current state was entered */
	float StateTotalSeconds;

	/* Seconds an alert guard has gone without line of sight */
	float SecondsWithoutSight;

	/* Seconds of stun left */
	float StunSecondsRemaining;

	// Patrol
	/* Patrol points relative to where the guard is placed. The
	   guard walks from its placed spot through each in turn. */
	UPROPERTY(EditAnywhere, Category = "Custom|Patrol",
		meta = (MakeEditWidget))
	TArray<FVector> PatrolPoints;

	/* World locations this guard walks between */
	TArray<FVector> PatrolLocations;

	/* Index of the patrol point being walked to */
	int32 PatrolLocationIndex;

	// Movement
	/* Location of the current move request */
	FVector CurrentMoveGoal;

	/* Whether a move request is active */
	bool bHasMoveGoal;

	/* Whether pathfinding failed and the guard walks straight */
	bool bIsSteeringDirectly;

	/* World time of the last path request */
	float LastPathRequestTimeSeconds;

	// Shooting
	/* Whether the laser is winding up for a shot */
	bool bIsWindingUpShot;

	/* Seconds the current windup has run */
	float ShotWindupElapsedSeconds;

	/* Seconds until the next shot may start */
	float ShotCooldownSecondsRemaining;

	/* Point the shot is aimed at */
	FVector ShotAimLocation;

	/* Timer that hides the laser after the firing flash */
	FTimerHandle LaserHideTimerHandle;

	// Cached Vars
	/* The player character */
	TWeakObjectPtr<ADirectorCharacter> CachedPlayer;
};
