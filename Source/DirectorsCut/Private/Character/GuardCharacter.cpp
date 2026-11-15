#include "Character/GuardCharacter.h"

#include "AI/GuardAIController.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Character/DirectorCharacter.h"
#include "Component/BeamMeshComponent.h"
#include "Component/DirectorHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Game/BacklotGameState.h"
#include "Game/GreyboxShapeLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace GuardConstants
{
	/* Distance at which a guard counts as having arrived */
	constexpr float ArrivalDistance{130.0f};

	/* A new path is requested when the goal moves this far */
	constexpr float PathGoalTolerance{100.0f};

	/* Seconds between repeated path requests to one goal */
	constexpr float PathRetrySeconds{0.5f};

	/* Longest a guard walks toward a spot it cannot reach */
	constexpr float MaximumInvestigateTravelSeconds{10.0f};

	/* Degrees per second a guard turns while looking around */
	constexpr float LookAroundYawRate{70.0f};

	/* Player speed above which footsteps can be heard */
	constexpr float AudibleFootstepSpeed{300.0f};

	/* Meter value a guard keeps when it starts searching */
	constexpr float SearchDetectionMeter{0.6f};

	/* Seconds an alerted guard needs before its first shot */
	constexpr float AlertReactionSeconds{0.6f};

	/* Farthest a shot travels */
	constexpr float ShotMaximumDistance{4000.0f};

	/* Seconds the bright firing flash of the laser stays up */
	constexpr float LaserFlashSeconds{0.12f};

	/* Laser thickness while aiming and while firing */
	constexpr float LaserAimThickness{3.0f};
	constexpr float LaserFireThickness{12.0f};

	/* Upward speed of a guard being reeled in */
	constexpr float GrapplePullUpwardSpeed{380.0f};

	/* Fraction of the way to the director a reeled-in guard
	   flies before landing */
	constexpr float GrapplePullLandingFraction{0.8f};

	/* Seconds a knocked-out guard stays in the level */
	constexpr float KnockedOutBodySeconds{20.0f};

	/* Number of straight pieces in the drawn vision cone arc */
	constexpr int32 DebugFanSegmentCount{8};

	/* Returns the marker and cone color for a guard state */
	FLinearColor GetStateColor(EGuardState GuardState)
	{
		switch (GuardState)
		{
		case EGuardState::Suspicious:
			return FLinearColor{1.0f, 0.85f, 0.0f};
		case EGuardState::Alert:
			return FLinearColor{1.0f, 0.05f, 0.05f};
		case EGuardState::Search:
			return FLinearColor{1.0f, 0.4f, 0.0f};
		case EGuardState::Stunned:
			return FLinearColor{0.7f, 0.8f, 1.0f};
		default:
			return FLinearColor{0.1f, 0.7f, 0.2f};
		}
	}
}

AGuardCharacter::AGuardCharacter()
	: CurrentState{EGuardState::Patrol},
	  StateBeforeStun{EGuardState::Patrol},
	  DetectionMeter{0.0f},
	  bCanSeePlayer{false},
	  bIsRelentless{false},
	  LastKnownPlayerLocation{FVector::ZeroVector},
	  StateElapsedSeconds{0.0f},
	  StateTotalSeconds{0.0f},
	  SecondsWithoutSight{0.0f},
	  StunSecondsRemaining{0.0f},
	  PatrolLocationIndex{0},
	  CurrentMoveGoal{FVector::ZeroVector},
	  bHasMoveGoal{false},
	  bIsSteeringDirectly{false},
	  LastPathRequestTimeSeconds{0.0f},
	  bIsWindingUpShot{false},
	  ShotWindupElapsedSeconds{0.0f},
	  ShotCooldownSecondsRemaining{0.0f},
	  ShotAimLocation{FVector::ZeroVector}
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AGuardAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	UCharacterMovementComponent* Movement{
		GetCharacterMovement()};
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator{0.0f, 360.0f, 0.0f};
	Movement->GetNavMovementProperties()
		->bUseAccelerationForPaths = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		GuardMeshFinder{TEXT(
			"/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
			".SKM_Manny_Simple")};
	static ConstructorHelpers::FClassFinder<UAnimInstance>
		AnimationFinder{TEXT(
			"/Game/Characters/Mannequins/Anims/Unarmed"
			"/ABP_Unarmed")};
	GetMesh()->SetSkeletalMesh(GuardMeshFinder.Object);
	GetMesh()->SetAnimInstanceClass(AnimationFinder.Class);
	GetMesh()->SetRelativeLocationAndRotation(
		FVector{0.0f, 0.0f, -90.0f},
		FRotator{0.0f, -90.0f, 0.0f});

	StateMarker = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("StateMarker"));
	StateMarker->SetupAttachment(GetCapsuleComponent());
	StateMarker->SetRelativeLocation(FVector{0.0f, 0.0f, 125.0f});
	StateMarker->SetRelativeScale3D(FVector{0.22f});
	StateMarker->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	StateMarker->SetCanEverAffectNavigation(false);
	StateMarker->SetCastShadow(false);

	LaserBeam = CreateDefaultSubobject<UBeamMeshComponent>(
		TEXT("LaserBeam"));
	LaserBeam->SetupAttachment(GetCapsuleComponent());
}

void AGuardCharacter::BeginPlay()
{
	Super::BeginPlay();
	StateMarker->SetStaticMesh(
		GreyboxShapeLibrary::GetSphereMesh());
	StateMarkerMaterial = GreyboxShapeLibrary::ApplyGlowColor(
		StateMarker,
		GuardConstants::GetStateColor(EGuardState::Patrol));
	LaserBeam->SetBeamThickness(
		GuardConstants::LaserAimThickness);
	LastKnownPlayerLocation = GetActorLocation();
	EnterState(EGuardState::Patrol);
}

void AGuardCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StateTotalSeconds += DeltaSeconds;
	UpdateDetectionMeter(DeltaSeconds);
	switch (CurrentState)
	{
	case EGuardState::Patrol:
		UpdatePatrol(DeltaSeconds);
		break;
	case EGuardState::Suspicious:
		UpdateSuspicious(DeltaSeconds);
		break;
	case EGuardState::Alert:
		UpdateAlert(DeltaSeconds);
		break;
	case EGuardState::Search:
		UpdateSearch(DeltaSeconds);
		break;
	case EGuardState::Stunned:
		UpdateStunned(DeltaSeconds);
		break;
	default:
		break;
	}
	DrawDebugVisionCone();
}

void AGuardCharacter::UpdateDetectionMeter(float DeltaSeconds)
{
	const bool bIsWatching{CurrentState == EGuardState::Patrol
		|| CurrentState == EGuardState::Suspicious
		|| CurrentState == EGuardState::Search};
	const ADirectorCharacter* Player{GetPlayer()};
	if (!bIsWatching || !Player)
	{
		return;
	}
	const float GainRate{GetSightDetectionRate(*Player)
		+ GetHearingDetectionRate(*Player)};
	if (GainRate <= 0.0f)
	{
		DetectionMeter = FMath::Max(0.0f, DetectionMeter
			- UBacklotTuningSettings::Get()
				.GuardDetectionDecayPerSecond * DeltaSeconds);
		return;
	}
	DetectionMeter = FMath::Min(1.0f,
		DetectionMeter + GainRate * DeltaSeconds);
	LastKnownPlayerLocation = Player->GetActorLocation();
	ReactToDetectionMeter();
}

void AGuardCharacter::ReactToDetectionMeter()
{
	if (DetectionMeter >= 1.0f)
	{
		BecomeAlert(LastKnownPlayerLocation);
		return;
	}
	const bool bIsSuspiciousEnough{DetectionMeter
		>= UBacklotTuningSettings::Get().GuardSuspiciousThreshold};
	if (bIsSuspiciousEnough
		&& CurrentState == EGuardState::Patrol)
	{
		EnterState(EGuardState::Suspicious);
	}
}

float AGuardCharacter::GetSightDetectionRate(
	const ADirectorCharacter& Player) const
{
	if (!bCanSeePlayer)
	{
		return 0.0f;
	}
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const float Distance{static_cast<float>(FVector::Dist(
		GetActorLocation(), Player.GetActorLocation()))};
	const bool bIsPlayerCrouched{Player.IsCrouched()};
	if (bIsPlayerCrouched && Distance > Tuning.GuardSightRange
		* Tuning.CrouchedSightRangeMultiplier)
	{
		return 0.0f;
	}
	const float Closeness{1.0f - FMath::Clamp(
		Distance / Tuning.GuardSightRange, 0.0f, 1.0f)};
	const float StandingRate{Tuning.GuardDetectionRatePerSecond
		* FMath::Lerp(1.0f,
			Tuning.GuardCloseRangeDetectionMultiplier, Closeness)};
	return bIsPlayerCrouched
		? StandingRate * Tuning.CrouchedDetectionMultiplier
		: StandingRate;
}

float AGuardCharacter::GetHearingDetectionRate(
	const ADirectorCharacter& Player) const
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const bool bIsRunningOnGround{!Player.IsCrouched()
		&& Player.GetCharacterMovement()->IsMovingOnGround()
		&& Player.GetVelocity().Size2D()
			> GuardConstants::AudibleFootstepSpeed};
	const bool bIsInEarshot{FVector::Dist(GetActorLocation(),
		Player.GetActorLocation())
		<= Tuning.GuardFootstepHearingRadius};
	return bIsRunningOnGround && bIsInEarshot
		? Tuning.GuardHearingDetectionRatePerSecond
		: 0.0f;
}

void AGuardCharacter::EnterState(EGuardState NewState)
{
	CurrentState = NewState;
	StateElapsedSeconds = 0.0f;
	StateTotalSeconds = 0.0f;
	if (NewState != EGuardState::Alert)
	{
		CancelShot();
	}
	StopMoving();
	ApplySpeedForState();
	ApplyFacingModeForState();
	UpdateStateMarker();
}

void AGuardCharacter::UpdatePatrol(float DeltaSeconds)
{
	if (PatrolLocations.IsEmpty())
	{
		return;
	}
	const FVector& GoalLocation{
		PatrolLocations[PatrolLocationIndex]};
	if (!HasReachedLocation(GoalLocation))
	{
		MoveTowardLocation(GoalLocation);
		return;
	}
	StateElapsedSeconds += DeltaSeconds;
	if (StateElapsedSeconds
		>= UBacklotTuningSettings::Get().GuardPatrolWaitSeconds)
	{
		StateElapsedSeconds = 0.0f;
		PatrolLocationIndex =
			(PatrolLocationIndex + 1) % PatrolLocations.Num();
	}
}

void AGuardCharacter::UpdateSuspicious(float DeltaSeconds)
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const float LookSeconds{
		InvestigateLastKnownLocation(DeltaSeconds)};
	if (LookSeconds >= Tuning.GuardSuspiciousLookSeconds
		&& DetectionMeter < Tuning.GuardSuspiciousThreshold)
	{
		EnterState(EGuardState::Patrol);
	}
}

float AGuardCharacter::InvestigateLastKnownLocation(
	float DeltaSeconds)
{
	const bool bHasArrived{
		HasReachedLocation(LastKnownPlayerLocation)
		|| StateTotalSeconds
			> GuardConstants::MaximumInvestigateTravelSeconds};
	if (!bHasArrived)
	{
		MoveTowardLocation(LastKnownPlayerLocation);
		return 0.0f;
	}
	StopMoving();
	AddActorWorldRotation(FRotator{0.0f,
		GuardConstants::LookAroundYawRate * DeltaSeconds, 0.0f});
	StateElapsedSeconds += DeltaSeconds;
	return StateElapsedSeconds;
}

void AGuardCharacter::UpdateAlert(float DeltaSeconds)
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	ADirectorCharacter* Player{GetPlayer()};
	AAIController* GuardController{GetGuardController()};
	if (!Player || !GuardController)
	{
		return;
	}
	if (GuardController->GetFocusActor() != Player)
	{
		GuardController->SetFocus(Player);
	}
	const bool bHasLineOfSight{
		GuardController->LineOfSightTo(Player)};
	SecondsWithoutSight = bHasLineOfSight
		? 0.0f : SecondsWithoutSight + DeltaSeconds;
	if (bHasLineOfSight)
	{
		LastKnownPlayerLocation = Player->GetActorLocation();
	}
	if (bIsWindingUpShot)
	{
		UpdateShotWindup(DeltaSeconds);
		return;
	}
	if (SecondsWithoutSight > Tuning.GuardLoseTargetSeconds)
	{
		DetectionMeter = GuardConstants::SearchDetectionMeter;
		EnterState(EGuardState::Search);
		return;
	}
	ShotCooldownSecondsRemaining -= DeltaSeconds;
	const bool bIsInShootingRange{FVector::Dist(GetActorLocation(),
		Player->GetActorLocation()) <= Tuning.GuardShootingRange};
	if (!bHasLineOfSight || !bIsInShootingRange)
	{
		MoveTowardLocation(LastKnownPlayerLocation);
		return;
	}
	StopMoving();
	if (ShotCooldownSecondsRemaining <= 0.0f)
	{
		BeginShotWindup();
	}
}

void AGuardCharacter::UpdateSearch(float DeltaSeconds)
{
	const float LookSeconds{
		InvestigateLastKnownLocation(DeltaSeconds)};
	if (LookSeconds
		< UBacklotTuningSettings::Get().GuardSearchSeconds)
	{
		return;
	}
	const ADirectorCharacter* Player{GetPlayer()};
	if (bIsRelentless && Player)
	{
		BecomeAlert(Player->GetActorLocation());
		return;
	}
	DetectionMeter = 0.0f;
	EnterState(EGuardState::Patrol);
}

void AGuardCharacter::UpdateStunned(float DeltaSeconds)
{
	StunSecondsRemaining -= DeltaSeconds;
	if (StunSecondsRemaining > 0.0f)
	{
		return;
	}
	if (StateBeforeStun == EGuardState::Alert)
	{
		SecondsWithoutSight = 0.0f;
		EnterState(EGuardState::Alert);
		return;
	}
	DetectionMeter = FMath::Max(DetectionMeter,
		UBacklotTuningSettings::Get().GuardSuspiciousThreshold);
	EnterState(EGuardState::Suspicious);
}

void AGuardCharacter::MoveTowardLocation(
	const FVector& GoalLocation)
{
	if (!GetGuardController())
	{
		return;
	}
	if (ShouldRequestNewPath(GoalLocation))
	{
		RequestPathTo(GoalLocation);
	}
	if (bIsSteeringDirectly)
	{
		AddMovementInput((GoalLocation - GetActorLocation())
			.GetSafeNormal2D());
	}
}

bool AGuardCharacter::ShouldRequestNewPath(
	const FVector& GoalLocation) const
{
	const bool bGoalMoved{!bHasMoveGoal
		|| FVector::Dist(GoalLocation, CurrentMoveGoal)
			> GuardConstants::PathGoalTolerance};
	if (bGoalMoved)
	{
		return true;
	}
	const bool bIsIdle{GetGuardController()->GetMoveStatus()
		== EPathFollowingStatus::Idle};
	const bool bRetryIsDue{GetWorld()->GetTimeSeconds()
		- LastPathRequestTimeSeconds
		> GuardConstants::PathRetrySeconds};
	return bIsIdle && bRetryIsDue;
}

void AGuardCharacter::RequestPathTo(const FVector& GoalLocation)
{
	CurrentMoveGoal = GoalLocation;
	bHasMoveGoal = true;
	LastPathRequestTimeSeconds = GetWorld()->GetTimeSeconds();
	const float AcceptanceRadius{50.0f};
	const EPathFollowingRequestResult::Type RequestResult{
		GetGuardController()->MoveToLocation(GoalLocation,
			AcceptanceRadius, true, true, true, false)};
	bIsSteeringDirectly =
		RequestResult == EPathFollowingRequestResult::Failed;
}

void AGuardCharacter::StopMoving()
{
	if (AAIController* GuardController{GetGuardController()})
	{
		GuardController->StopMovement();
	}
	bHasMoveGoal = false;
	bIsSteeringDirectly = false;
}

bool AGuardCharacter::HasReachedLocation(
	const FVector& Location) const
{
	return FVector::Dist2D(GetActorLocation(), Location)
		<= GuardConstants::ArrivalDistance;
}

void AGuardCharacter::ApplySpeedForState()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	float StateSpeed{Tuning.GuardInvestigateSpeed};
	if (CurrentState == EGuardState::Patrol)
	{
		StateSpeed = Tuning.GuardPatrolSpeed;
	}
	else if (CurrentState == EGuardState::Alert)
	{
		StateSpeed = Tuning.GuardChaseSpeed;
	}
	GetCharacterMovement()->MaxWalkSpeed = StateSpeed;
}

void AGuardCharacter::ApplyFacingModeForState()
{
	const bool bShouldFacePlayer{
		CurrentState == EGuardState::Alert};
	UCharacterMovementComponent* Movement{
		GetCharacterMovement()};
	Movement->bOrientRotationToMovement = !bShouldFacePlayer;
	Movement->bUseControllerDesiredRotation = bShouldFacePlayer;

	AAIController* GuardController{GetGuardController()};
	if (!GuardController)
	{
		return;
	}
	if (bShouldFacePlayer)
	{
		GuardController->SetFocus(GetPlayer());
		return;
	}
	GuardController->ClearFocus(EAIFocusPriority::Gameplay);
}

void AGuardCharacter::BeginShotWindup()
{
	bIsWindingUpShot = true;
	ShotWindupElapsedSeconds = 0.0f;
	if (ADirectorCharacter* Player{GetPlayer()})
	{
		ShotAimLocation = Player->GetActorLocation();
		Player->GetHealthComponent()->DelayRegeneration();
	}
}

void AGuardCharacter::UpdateShotWindup(float DeltaSeconds)
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	ShotWindupElapsedSeconds += DeltaSeconds;
	const float SecondsUntilShot{
		Tuning.GuardShotWindupSeconds - ShotWindupElapsedSeconds};
	const bool bIsAimLocked{
		SecondsUntilShot <= Tuning.GuardAimLockSeconds};
	const ADirectorCharacter* Player{GetPlayer()};
	if (!bIsAimLocked && Player)
	{
		ShotAimLocation = Player->GetActorLocation();
	}
	LaserBeam->SetBeamColor(bIsAimLocked
		? FLinearColor{1.0f, 0.0f, 0.0f}
		: FLinearColor{1.0f, 0.5f, 0.0f});
	LaserBeam->ShowBeamBetween(GetMuzzleLocation(),
		GetShotEndLocation());
	if (SecondsUntilShot <= 0.0f)
	{
		FireShot();
	}
}

void AGuardCharacter::FireShot()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	bIsWindingUpShot = false;
	ShotCooldownSecondsRemaining = Tuning.GuardShotCooldownSeconds;

	const FVector ShotStart{GetMuzzleLocation()};
	const FVector ShotEnd{GetShotEndLocation()};
	ADirectorCharacter* Player{GetPlayer()};
	if (Player && FMath::PointDistToSegment(
		Player->GetActorLocation(), ShotStart, ShotEnd)
		<= Tuning.GuardShotHitRadius)
	{
		Player->ReceiveGuardShot(ShotStart);
	}

	LaserBeam->SetBeamThickness(
		GuardConstants::LaserFireThickness);
	LaserBeam->SetBeamColor(FLinearColor::White);
	LaserBeam->ShowBeamBetween(ShotStart, ShotEnd);
	GetWorldTimerManager().SetTimer(LaserHideTimerHandle, this,
		&AGuardCharacter::HideLaser,
		GuardConstants::LaserFlashSeconds);
}

void AGuardCharacter::CancelShot()
{
	bIsWindingUpShot = false;
	GetWorldTimerManager().ClearTimer(LaserHideTimerHandle);
	HideLaser();
}

void AGuardCharacter::HideLaser()
{
	LaserBeam->HideBeam();
	LaserBeam->SetBeamThickness(
		GuardConstants::LaserAimThickness);
}

FVector AGuardCharacter::GetMuzzleLocation() const
{
	return GetActorLocation()
		+ GetActorForwardVector() * 45.0f
		+ FVector{0.0f, 0.0f, 45.0f};
}

FVector AGuardCharacter::GetShotEndLocation() const
{
	const FVector ShotStart{GetMuzzleLocation()};
	const FVector ShotDirection{
		(ShotAimLocation - ShotStart).GetSafeNormal()};
	const FVector RangeEnd{ShotStart
		+ ShotDirection * GuardConstants::ShotMaximumDistance};
	FHitResult Hit;
	const bool bHitWall{GetWorld()->LineTraceSingleByChannel(
		Hit, ShotStart, RangeEnd, ECC_Visibility)};
	return bHitWall ? Hit.ImpactPoint : RangeEnd;
}

void AGuardCharacter::UpdateStateMarker()
{
	if (StateMarkerMaterial)
	{
		StateMarkerMaterial->SetVectorParameterValue(
			TEXT("Color"),
			GuardConstants::GetStateColor(CurrentState));
	}
}

void AGuardCharacter::DrawDebugVisionCone() const
{
	const ABacklotGameState* GameState{
		GetWorld()->GetGameState<ABacklotGameState>()};
	if (!GameState || !GameState->ShouldDrawDebugVisionCones())
	{
		return;
	}
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const FVector FanOrigin{GetActorLocation()};
	const FColor FanColor{GuardConstants::GetStateColor(
		CurrentState).ToFColor(true)};
	const float HalfAngle{Tuning.GuardVisionConeHalfAngleDegrees};
	const float SegmentAngle{HalfAngle * 2.0f
		/ GuardConstants::DebugFanSegmentCount};
	FVector PreviousPoint{FanOrigin};
	for (int32 PointIndex{0};
		PointIndex <= GuardConstants::DebugFanSegmentCount;
		++PointIndex)
	{
		const FRotator PointRotation{0.0f, GetActorRotation().Yaw
			- HalfAngle + SegmentAngle * PointIndex, 0.0f};
		const FVector FanPoint{FanOrigin
			+ PointRotation.Vector() * Tuning.GuardSightRange};
		DrawDebugLine(GetWorld(), PreviousPoint, FanPoint,
			FanColor, false, -1.0f, 0, 2.0f);
		PreviousPoint = FanPoint;
	}
	DrawDebugLine(GetWorld(), PreviousPoint, FanOrigin, FanColor,
		false, -1.0f, 0, 2.0f);
}

void AGuardCharacter::SetPatrolLocations(
	const TArray<FVector>& InPatrolLocations)
{
	PatrolLocations = InPatrolLocations;
	PatrolLocationIndex = 0;
}

void AGuardCharacter::MakeRelentless()
{
	bIsRelentless = true;
}

void AGuardCharacter::HandlePlayerSightChanged(
	bool bInCanSeePlayer)
{
	bCanSeePlayer = bInCanSeePlayer;
}

void AGuardCharacter::ApplyStun(float StunSeconds)
{
	if (CurrentState == EGuardState::TakenDown)
	{
		return;
	}
	if (CurrentState != EGuardState::Stunned)
	{
		StateBeforeStun = CurrentState;
	}
	if (const ADirectorCharacter* Player{GetPlayer()})
	{
		LastKnownPlayerLocation = Player->GetActorLocation();
	}
	StunSecondsRemaining = StunSeconds;
	EnterState(EGuardState::Stunned);
}

void AGuardCharacter::ReceiveTakedown()
{
	if (CurrentState == EGuardState::TakenDown)
	{
		return;
	}
	EnterState(EGuardState::TakenDown);
	SetActorTickEnabled(false);
	DetachFromControllerPendingDestroy();
	StateMarker->SetHiddenInGame(true);
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	SetLifeSpan(GuardConstants::KnockedOutBodySeconds);
}

void AGuardCharacter::ReceiveGrapplePull(
	const FVector& PullerLocation)
{
	if (CurrentState == EGuardState::TakenDown)
	{
		return;
	}
	ReceiveTakedown();
	const FVector ToPuller{PullerLocation - GetActorLocation()};
	const float FlightSeconds{2.0f
		* GuardConstants::GrapplePullUpwardSpeed
		/ FMath::Abs(GetWorld()->GetGravityZ())};
	const float HorizontalSpeed{static_cast<float>(
		ToPuller.Size2D()) * GuardConstants::GrapplePullLandingFraction
		/ FlightSeconds};
	GetMesh()->AddImpulse(
		ToPuller.GetSafeNormal2D() * HorizontalSpeed
		+ FVector{0.0f, 0.0f,
			GuardConstants::GrapplePullUpwardSpeed},
		NAME_None, true);
}

void AGuardCharacter::BecomeAlert(
	const FVector& KnownPlayerLocation)
{
	if (CurrentState == EGuardState::TakenDown)
	{
		return;
	}
	LastKnownPlayerLocation = KnownPlayerLocation;
	if (CurrentState == EGuardState::Stunned)
	{
		StateBeforeStun = EGuardState::Alert;
		return;
	}
	if (CurrentState == EGuardState::Alert)
	{
		return;
	}
	DetectionMeter = 1.0f;
	SecondsWithoutSight = 0.0f;
	ShotCooldownSecondsRemaining =
		GuardConstants::AlertReactionSeconds;
	EnterState(EGuardState::Alert);
	ShoutToNearbyGuards();
}

void AGuardCharacter::ShoutToNearbyGuards() const
{
	const float ShoutRadius{
		UBacklotTuningSettings::Get().GuardAlertShoutRadius};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		AGuardCharacter* OtherGuard{*GuardIterator};
		const bool bIsInEarshot{FVector::Dist(GetActorLocation(),
			OtherGuard->GetActorLocation()) <= ShoutRadius};
		if (OtherGuard != this && bIsInEarshot)
		{
			OtherGuard->InvestigateLocation(
				LastKnownPlayerLocation);
		}
	}
}

void AGuardCharacter::InvestigateLocation(const FVector& Location)
{
	const bool bCanInvestigate{
		CurrentState == EGuardState::Patrol
		|| CurrentState == EGuardState::Suspicious
		|| CurrentState == EGuardState::Search};
	if (!bCanInvestigate)
	{
		return;
	}
	LastKnownPlayerLocation = Location;
	DetectionMeter = FMath::Max(DetectionMeter,
		UBacklotTuningSettings::Get().GuardSuspiciousThreshold);
	EnterState(CurrentState == EGuardState::Search
		? EGuardState::Search : EGuardState::Suspicious);
}

void AGuardCharacter::ResetToPatrolStart()
{
	if (CurrentState == EGuardState::TakenDown)
	{
		return;
	}
	DetectionMeter = 0.0f;
	PatrolLocationIndex = 0;
	if (!PatrolLocations.IsEmpty())
	{
		SetActorLocation(PatrolLocations[0]);
	}
	EnterState(EGuardState::Patrol);
}

EGuardState AGuardCharacter::GetGuardState() const
{
	return CurrentState;
}

float AGuardCharacter::GetDetectionMeter() const
{
	return DetectionMeter;
}

bool AGuardCharacter::IsStunned() const
{
	return CurrentState == EGuardState::Stunned;
}

bool AGuardCharacter::IsAlert() const
{
	return CurrentState == EGuardState::Alert;
}

bool AGuardCharacter::IsTakenDown() const
{
	return CurrentState == EGuardState::TakenDown;
}

bool AGuardCharacter::IsFacingLocation(
	const FVector& Location) const
{
	const FVector DirectionToLocation{
		(Location - GetActorLocation()).GetSafeNormal2D()};
	return FVector::DotProduct(GetActorForwardVector(),
		DirectionToLocation)
		> UBacklotTuningSettings::Get().GuardFacingDotThreshold;
}

ADirectorCharacter* AGuardCharacter::GetPlayer()
{
	if (!CachedPlayer.IsValid())
	{
		CachedPlayer = Cast<ADirectorCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0));
	}
	return CachedPlayer.Get();
}

AAIController* AGuardCharacter::GetGuardController() const
{
	return Cast<AAIController>(GetController());
}
