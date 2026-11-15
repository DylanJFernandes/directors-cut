#include "Data/BacklotTuningSettings.h"

UBacklotTuningSettings::UBacklotTuningSettings()
	: PlayerRunSpeed{600.0f},
	  PlayerCrouchSpeed{280.0f},
	  PlayerJumpVelocity{620.0f},
	  PlayerAirControl{0.45f},
	  CameraBoomLength{420.0f},
	  LookSensitivity{0.5f},
	  PlayerMaximumHealth{3},
	  HitInvulnerabilitySeconds{0.6f},
	  HealthRegenerationDelaySeconds{6.0f},
	  HealthRegenerationIntervalSeconds{4.0f},
	  GrappleRange{2500.0f},
	  GrappleAimConeHalfAngleDegrees{30.0f},
	  GrappleTravelSpeed{2400.0f},
	  GrappleMinimumTravelSeconds{0.35f},
	  GrappleArcHeight{220.0f},
	  GrappleExitForwardSpeed{120.0f},
	  GrappleExitUpwardSpeed{150.0f},
	  GuardPatrolSpeed{170.0f},
	  GuardInvestigateSpeed{260.0f},
	  GuardChaseSpeed{430.0f},
	  GuardPatrolWaitSeconds{1.5f},
	  GuardSightRange{2000.0f},
	  GuardVisionConeHalfAngleDegrees{35.0f},
	  GuardDetectionRatePerSecond{0.55f},
	  GuardCloseRangeDetectionMultiplier{3.0f},
	  CrouchedDetectionMultiplier{0.35f},
	  CrouchedSightRangeMultiplier{0.6f},
	  GuardDetectionDecayPerSecond{0.2f},
	  GuardSuspiciousThreshold{0.35f},
	  GuardFootstepHearingRadius{450.0f},
	  GuardHearingDetectionRatePerSecond{0.6f},
	  GuardSuspiciousLookSeconds{3.5f},
	  GuardSearchSeconds{6.0f},
	  GuardLoseTargetSeconds{4.0f},
	  GuardAlertShoutRadius{1100.0f},
	  GuardShootingRange{1500.0f},
	  GuardShotWindupSeconds{1.0f},
	  GuardAimLockSeconds{0.35f},
	  GuardShotCooldownSeconds{1.6f},
	  GuardShotHitRadius{50.0f},
	  FlashRange{1200.0f},
	  FlashConeHalfAngleDegrees{45.0f},
	  FlashStunSeconds{3.0f},
	  FlashCooldownSeconds{8.0f},
	  TakedownRange{230.0f},
	  GuardFacingDotThreshold{0.3f},
	  AerialTakedownHorizontalRange{350.0f},
	  AerialTakedownMaximumDrop{900.0f},
	  FailedTakedownKnockbackSpeed{750.0f},
	  MegaphoneRange{3000.0f},
	  MegaphoneNoiseRadius{1400.0f},
	  MegaphoneCooldownSeconds{5.0f},
	  UploadDurationSeconds{90.0f},
	  UploadMaximumPlayerDistance{800.0f},
	  UploadInteractionRange{300.0f},
	  UploadFirstWaveDelaySeconds{4.0f},
	  UploadWaveIntervalSeconds{16.0f},
	  UploadFirstWaveGuardCount{2},
	  UploadWaveGuardGrowth{1},
	  UploadMaximumWaveGuardCount{5},
	  UploadMaximumLivingGuards{7}
{
}

const UBacklotTuningSettings& UBacklotTuningSettings::Get()
{
	return *GetDefault<UBacklotTuningSettings>();
}

FName UBacklotTuningSettings::GetCategoryName() const
{
	return TEXT("Game");
}
