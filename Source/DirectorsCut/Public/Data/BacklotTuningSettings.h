#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BacklotTuningSettings.generated.h"

/* Every gameplay tuning value in one place. Edit them in
   Project Settings > Game > Director's Cut Tuning; no recompile
   is needed and changes apply to the next Play session. */
UCLASS(Config = Game, DefaultConfig,
	meta = (DisplayName = "Director's Cut Tuning"))
class UBacklotTuningSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/* Sets the default value of every tuning number */
	UBacklotTuningSettings();

	/* Returns the tuning values currently set in the project */
	static const UBacklotTuningSettings& Get();

	/* Places these settings under the Game section */
	virtual FName GetCategoryName() const override;

	// Player
	/* Ground speed while standing, in units per second */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Player")
	float PlayerRunSpeed;

	/* Ground speed while crouched, in units per second */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Player")
	float PlayerCrouchSpeed;

	/* Upward speed applied when the player jumps */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Player")
	float PlayerJumpVelocity;

	/* How much the player can steer while airborne (0 to 1) */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Player")
	float PlayerAirControl;

	/* Distance from the player to the third person camera */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Player")
	float CameraBoomLength;

	/* Degrees of camera turn per unit of mouse movement */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Player")
	float LookSensitivity;

	// Health
	/* Number of guard hits the player can take */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Health")
	int32 PlayerMaximumHealth;

	/* Seconds after a hit during which new hits are ignored */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Health")
	float HitInvulnerabilitySeconds;

	/* Seconds out of combat before health starts coming back */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Health")
	float HealthRegenerationDelaySeconds;

	/* Seconds between each regenerated point of health */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Health")
	float HealthRegenerationIntervalSeconds;

	// Grapple
	/* Farthest hook point the film reel can reach */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleRange;

	/* Hook points farther than this from screen center, in
	   degrees, are not targeted */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleAimConeHalfAngleDegrees;

	/* Travel speed along the grapple arc, in units per second */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleTravelSpeed;

	/* Shortest time a grapple can take, so short hops still
	   read as an arc */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleMinimumTravelSeconds;

	/* Extra height at the top of the grapple arc */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleArcHeight;

	/* Forward speed the player keeps when the grapple ends */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleExitForwardSpeed;

	/* Upward speed the player keeps when the grapple ends */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrappleExitUpwardSpeed;

	/* Farthest stunned guard the film reel can reel in */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrapplePullRange;

	/* A stunned guard must be this close to screen center, in
	   degrees, to be reeled in */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Grapple")
	float GrapplePullAimConeHalfAngleDegrees;

	// Guard Movement
	/* Guard walking speed while patrolling */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Movement")
	float GuardPatrolSpeed;

	/* Guard speed while investigating or searching */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Movement")
	float GuardInvestigateSpeed;

	/* Guard speed while chasing the player */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Movement")
	float GuardChaseSpeed;

	/* Seconds a guard waits at each patrol point */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Movement")
	float GuardPatrolWaitSeconds;

	// Guard Detection
	/* How far a guard can see */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardSightRange;

	/* Half of the guard vision cone angle (35 = a 70 degree
	   cone) */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardVisionConeHalfAngleDegrees;

	/* Detection meter gained per second at maximum sight range
	   while the player stands */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardDetectionRatePerSecond;

	/* Detection rate multiplier when the player is right next
	   to the guard */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardCloseRangeDetectionMultiplier;

	/* Detection rate multiplier while the player crouches */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float CrouchedDetectionMultiplier;

	/* Fraction of the sight range at which a crouched player
	   can still be seen */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float CrouchedSightRangeMultiplier;

	/* Detection meter lost per second while the player is
	   hidden */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardDetectionDecayPerSecond;

	/* Detection meter value at which a guard gets suspicious */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardSuspiciousThreshold;

	/* Distance at which guards hear a running player */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardFootstepHearingRadius;

	/* Detection meter gained per second from hearing footsteps */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardHearingDetectionRatePerSecond;

	/* Seconds a suspicious guard looks around before giving up */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardSuspiciousLookSeconds;

	/* Seconds a guard searches after losing the player */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardSearchSeconds;

	/* Seconds without line of sight before an alert guard
	   starts searching */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardLoseTargetSeconds;

	/* Distance at which an alerted guard warns other guards */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Detection")
	float GuardAlertShoutRadius;

	// Guard Combat
	/* Distance inside which an alert guard stops and shoots */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Combat")
	float GuardShootingRange;

	/* Seconds the laser telegraph shows before a shot fires */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Combat")
	float GuardShotWindupSeconds;

	/* Seconds before firing at which the aim stops tracking
	   the player, giving a window to dodge */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Combat")
	float GuardAimLockSeconds;

	/* Seconds between a guard's shots */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Combat")
	float GuardShotCooldownSeconds;

	/* How close the shot line must pass to the player to hit */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Guard Combat")
	float GuardShotHitRadius;

	// Camera Flash
	/* How far the camera flash reaches */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Camera Flash")
	float FlashRange;

	/* Half of the camera flash cone angle, in degrees */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Camera Flash")
	float FlashConeHalfAngleDegrees;

	/* Seconds a flashed guard stays stunned */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Camera Flash")
	float FlashStunSeconds;

	/* Seconds before the flash can be used again */
	UPROPERTY(Config, EditAnywhere,
		Category = "Custom|Camera Flash")
	float FlashCooldownSeconds;

	// Takedown
	/* Distance inside which a ground takedown can start */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float TakedownRange;

	/* A guard counts as facing the player when the dot product
	   of its forward and the direction to the player is above
	   this value */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float GuardFacingDotThreshold;

	/* Horizontal reach of a takedown started from above */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float AerialTakedownHorizontalRange;

	/* Largest drop a takedown from above can cover */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float AerialTakedownMaximumDrop;

	/* Speed the player is thrown back by a failed takedown */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float FailedTakedownKnockbackSpeed;

	/* Game speed during the dramatic cut after a takedown
	   (1 = no slow motion) */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float TakedownSlowMotionTimeDilation;

	/* Real seconds the dramatic cut lasts */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Takedown")
	float TakedownSlowMotionSeconds;

	// Megaphone
	/* How far away the megaphone noise can be placed */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Megaphone")
	float MegaphoneRange;

	/* Guards within this distance of the noise investigate it */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Megaphone")
	float MegaphoneNoiseRadius;

	/* Seconds before the megaphone can be used again */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Megaphone")
	float MegaphoneCooldownSeconds;

	// Camera Rig
	/* How far the camera rig's spotlight reaches */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Camera Rig")
	float TurretRange;

	/* A guard must be this close to screen center, in degrees,
	   for the spotlight to hit him */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Camera Rig")
	float TurretAimConeHalfAngleDegrees;

	/* Seconds a guard hit by the spotlight stays stunned */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Camera Rig")
	float TurretStunSeconds;

	/* Seconds between spotlight shots */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Camera Rig")
	float TurretFireIntervalSeconds;

	// Upload
	/* Seconds of uploading needed to win */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	float UploadDurationSeconds;

	/* The upload pauses when the player is farther than this
	   from the terminal */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	float UploadMaximumPlayerDistance;

	/* Distance inside which E starts the upload */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	float UploadInteractionRange;

	/* Seconds between the upload starting and the first wave */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	float UploadFirstWaveDelaySeconds;

	/* Seconds between guard waves */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	float UploadWaveIntervalSeconds;

	/* Number of guards in the first wave */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	int32 UploadFirstWaveGuardCount;

	/* Guards added to each following wave */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	int32 UploadWaveGuardGrowth;

	/* Largest number of guards a single wave can contain */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	int32 UploadMaximumWaveGuardCount;

	/* Waves stop adding guards while this many are still up */
	UPROPERTY(Config, EditAnywhere, Category = "Custom|Upload")
	int32 UploadMaximumLivingGuards;
};
