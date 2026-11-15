#include "Component/GrappleComponent.h"

#include "Actor/GrappleHookPoint.h"
#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

namespace GrappleConstants
{
	/* Targets closer than this are not worth a grapple */
	constexpr float MinimumGrappleDistance{200.0f};

	/* A grapple aimed at a guard ends this far from him */
	constexpr float GuardArrivalDistance{140.0f};
}

UGrappleComponent::UGrappleComponent()
	: bIsGrappling{false},
	  GrappleStartLocation{FVector::ZeroVector},
	  GrappleDestination{FVector::ZeroVector},
	  GrappleDurationSeconds{0.0f},
	  GrappleElapsedSeconds{0.0f},
	  LastGrappleStartTimeSeconds{-1000.0f}
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedOwnerCharacter = Cast<ACharacter>(GetOwner());
}

void UGrappleComponent::TickComponent(float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTargets();
	if (bIsGrappling)
	{
		AdvanceGrapple(DeltaTime);
	}
}

void UGrappleComponent::UpdateTargets()
{
	AGrappleHookPoint* BestHookPoint{nullptr};
	TargetedGuard.Reset();
	if (CachedOwnerCharacter.IsValid()
		&& CachedOwnerCharacter->GetController())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		CachedOwnerCharacter->GetController()->GetPlayerViewPoint(
			ViewLocation, ViewRotation);
		TargetedGuard = FindBestGuard(ViewLocation,
			ViewRotation.Vector());
		BestHookPoint = TargetedGuard.IsValid() ? nullptr
			: FindBestHookPoint(ViewLocation,
				ViewRotation.Vector());
	}
	if (BestHookPoint == TargetedHookPoint.Get())
	{
		return;
	}
	if (TargetedHookPoint.IsValid())
	{
		TargetedHookPoint->ShowAsIdle();
	}
	TargetedHookPoint = BestHookPoint;
	if (BestHookPoint)
	{
		BestHookPoint->ShowAsTargeted();
	}
}

AGrappleHookPoint* UGrappleComponent::FindBestHookPoint(
	const FVector& ViewLocation,
	const FVector& ViewDirection) const
{
	AGrappleHookPoint* BestHookPoint{nullptr};
	float BestAimAngle{UBacklotTuningSettings::Get()
		.GrappleAimConeHalfAngleDegrees};
	for (TActorIterator<AGrappleHookPoint> HookPointIterator{
			GetWorld()};
		HookPointIterator; ++HookPointIterator)
	{
		const float AimAngle{GetAimAngleToHookPoint(
			**HookPointIterator, ViewLocation, ViewDirection)};
		if (AimAngle >= 0.0f && AimAngle < BestAimAngle)
		{
			BestAimAngle = AimAngle;
			BestHookPoint = *HookPointIterator;
		}
	}
	return BestHookPoint;
}

AGuardCharacter* UGrappleComponent::FindBestGuard(
	const FVector& ViewLocation,
	const FVector& ViewDirection) const
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	AGuardCharacter* BestGuard{nullptr};
	float BestAimAngle{Tuning.GrappleGuardAimConeHalfAngleDegrees};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		const FVector GuardLocation{
			GuardIterator->GetActorLocation()};
		const bool bCanBeGrappled{!GuardIterator->IsTakenDown()
			&& *GuardIterator != DestinationGuard.Get()
			&& IsWithinGrappleDistance(GuardLocation,
				Tuning.GrappleGuardRange)};
		const float AimAngle{GetAimAngleToLocation(GuardLocation,
			ViewLocation, ViewDirection)};
		if (bCanBeGrappled && AimAngle < BestAimAngle
			&& HasLineOfSight(ViewLocation, GuardLocation))
		{
			BestAimAngle = AimAngle;
			BestGuard = *GuardIterator;
		}
	}
	return BestGuard;
}

float UGrappleComponent::GetAimAngleToHookPoint(
	const AGrappleHookPoint& HookPoint,
	const FVector& ViewLocation,
	const FVector& ViewDirection) const
{
	const float NotGrappleable{-1.0f};
	const FVector HookLocation{HookPoint.GetActorLocation()};
	const bool bCanBeGrappled{
		&HookPoint != DestinationHookPoint.Get()
		&& IsWithinGrappleDistance(HookLocation,
			UBacklotTuningSettings::Get().GrappleRange)
		&& HasLineOfSight(ViewLocation, HookLocation)};
	if (!bCanBeGrappled)
	{
		return NotGrappleable;
	}
	return GetAimAngleToLocation(HookLocation, ViewLocation,
		ViewDirection);
}

float UGrappleComponent::GetAimAngleToLocation(
	const FVector& Location, const FVector& ViewLocation,
	const FVector& ViewDirection) const
{
	const FVector DirectionToLocation{
		(Location - ViewLocation).GetSafeNormal()};
	const float AimDot{static_cast<float>(
		FVector::DotProduct(ViewDirection, DirectionToLocation))};
	return FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(AimDot, -1.0f, 1.0f)));
}

bool UGrappleComponent::IsWithinGrappleDistance(
	const FVector& Location, float MaximumDistance) const
{
	const float DistanceFromOwner{static_cast<float>(
		FVector::Dist(Location,
			CachedOwnerCharacter->GetActorLocation()))};
	return DistanceFromOwner <= MaximumDistance
		&& DistanceFromOwner
			>= GrappleConstants::MinimumGrappleDistance;
}

bool UGrappleComponent::HasLineOfSight(
	const FVector& ViewLocation,
	const FVector& TargetLocation) const
{
	FCollisionQueryParams QueryParams{
		TEXT("GrappleLineOfSight"), false, GetOwner()};
	FHitResult Hit;
	const bool bIsBlocked{GetWorld()->LineTraceSingleByChannel(
		Hit, ViewLocation, TargetLocation, ECC_Visibility,
		QueryParams)};
	return !bIsBlocked;
}

bool UGrappleComponent::TryStartGrapple()
{
	const bool bHasTarget{TargetedGuard.IsValid()
		|| TargetedHookPoint.IsValid()};
	if (!bHasTarget || !CachedOwnerCharacter.IsValid()
		|| GetCooldownFractionRemaining() > 0.0f)
	{
		return false;
	}
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	DestinationGuard = TargetedGuard;
	DestinationHookPoint = TargetedHookPoint;
	GrappleStartLocation =
		CachedOwnerCharacter->GetActorLocation();
	GrappleDestination = TargetedGuard.IsValid()
		? TargetedGuard->GetActorLocation()
		: TargetedHookPoint->GetActorLocation();
	const float TravelDistance{static_cast<float>(FVector::Dist(
		GrappleStartLocation, GrappleDestination))};
	GrappleDurationSeconds = FMath::Max(
		Tuning.GrappleMinimumTravelSeconds,
		TravelDistance / Tuning.GrappleTravelSpeed);
	GrappleElapsedSeconds = 0.0f;
	LastGrappleStartTimeSeconds = GetWorld()->GetTimeSeconds();
	bIsGrappling = true;

	CachedOwnerCharacter->UnCrouch();
	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_Flying);
	return true;
}

void UGrappleComponent::AdvanceGrapple(float DeltaTime)
{
	if (!CachedOwnerCharacter.IsValid())
	{
		return;
	}
	const bool bIsChasingGuard{DestinationGuard.IsValid()
		&& !DestinationGuard->IsTakenDown()};
	if (bIsChasingGuard)
	{
		GrappleDestination = DestinationGuard->GetActorLocation();
	}
	GrappleElapsedSeconds += DeltaTime;
	const float Progress{FMath::Clamp(
		GrappleElapsedSeconds / GrappleDurationSeconds,
		0.0f, 1.0f)};

	const FVector ArcLocation{GetArcLocation(Progress)};
	CachedOwnerCharacter->SetActorLocation(ArcLocation);
	const bool bHasReachedGuard{bIsChasingGuard
		&& FVector::Dist(ArcLocation, GrappleDestination)
			<= GrappleConstants::GuardArrivalDistance};
	if (Progress >= 1.0f || bHasReachedGuard)
	{
		FinishGrapple();
	}
}

FVector UGrappleComponent::GetArcLocation(float Progress) const
{
	const float EasedProgress{
		FMath::InterpEaseInOut(0.0f, 1.0f, Progress, 2.0f)};
	const FVector StraightLocation{FMath::Lerp(
		GrappleStartLocation, GrappleDestination,
		EasedProgress)};
	const float ArcOffset{
		UBacklotTuningSettings::Get().GrappleArcHeight
		* FMath::Sin(PI * EasedProgress)};
	return StraightLocation + FVector{0.0f, 0.0f, ArcOffset};
}

void UGrappleComponent::FinishGrapple()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const FVector TravelDirection{
		(GrappleDestination - GrappleStartLocation)
			.GetSafeNormal2D()};
	AGuardCharacter* ReachedGuard{DestinationGuard.Get()};
	const bool bWasAimedAtGuard{ReachedGuard != nullptr};
	bIsGrappling = false;
	DestinationHookPoint.Reset();
	DestinationGuard.Reset();

	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = bWasAimedAtGuard
		? TravelDirection * Tuning.GrappleGuardExitForwardSpeed
			+ FVector{0.0f, 0.0f,
				Tuning.GrappleGuardExitUpwardSpeed}
		: TravelDirection * Tuning.GrappleExitForwardSpeed
			+ FVector{0.0f, 0.0f, Tuning.GrappleExitUpwardSpeed};
	if (ReachedGuard && !ReachedGuard->IsTakenDown())
	{
		OnGrappleReachedGuard.ExecuteIfBound(*ReachedGuard);
	}
}

void UGrappleComponent::CancelGrapple()
{
	if (!bIsGrappling || !CachedOwnerCharacter.IsValid())
	{
		return;
	}
	bIsGrappling = false;
	DestinationHookPoint.Reset();
	DestinationGuard.Reset();
	CachedOwnerCharacter->GetCharacterMovement()
		->SetMovementMode(MOVE_Falling);
}

AGrappleHookPoint* UGrappleComponent::GetTargetedHookPoint() const
{
	return TargetedHookPoint.Get();
}

AGuardCharacter* UGrappleComponent::GetTargetedGuard() const
{
	return TargetedGuard.Get();
}

bool UGrappleComponent::IsGrappling() const
{
	return bIsGrappling;
}

FVector UGrappleComponent::GetGrappleDestination() const
{
	return GrappleDestination;
}

float UGrappleComponent::GetCooldownFractionRemaining() const
{
	const float CooldownSeconds{
		UBacklotTuningSettings::Get().GrappleCooldownSeconds};
	if (CooldownSeconds <= 0.0f)
	{
		return 0.0f;
	}
	const float SecondsSinceGrapple{static_cast<float>(
		GetWorld()->GetTimeSeconds()
			- LastGrappleStartTimeSeconds)};
	return FMath::Clamp(
		1.0f - SecondsSinceGrapple / CooldownSeconds, 0.0f, 1.0f);
}
