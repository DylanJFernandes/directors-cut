#include "Component/GrappleComponent.h"

#include "Actor/GrappleHookPoint.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

namespace GrappleConstants
{
	/* Hook points closer than this are not worth a grapple */
	constexpr float MinimumGrappleDistance{200.0f};

}

UGrappleComponent::UGrappleComponent()
	: bIsGrappling{false},
	  GrappleStartLocation{FVector::ZeroVector},
	  GrappleDestination{FVector::ZeroVector},
	  GrappleDurationSeconds{0.0f},
	  GrappleElapsedSeconds{0.0f}
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
	UpdateTargetedHookPoint();
	if (bIsGrappling)
	{
		AdvanceGrapple(DeltaTime);
	}
}

void UGrappleComponent::UpdateTargetedHookPoint()
{
	AGrappleHookPoint* BestHookPoint{FindBestHookPoint()};
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

AGrappleHookPoint* UGrappleComponent::FindBestHookPoint() const
{
	if (!CachedOwnerCharacter.IsValid()
		|| !CachedOwnerCharacter->GetController())
	{
		return nullptr;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	CachedOwnerCharacter->GetController()->GetPlayerViewPoint(
		ViewLocation, ViewRotation);
	const FVector ViewDirection{ViewRotation.Vector()};

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

float UGrappleComponent::GetAimAngleToHookPoint(
	const AGrappleHookPoint& HookPoint,
	const FVector& ViewLocation,
	const FVector& ViewDirection) const
{
	const float NotGrappleable{-1.0f};
	if (&HookPoint == DestinationHookPoint.Get())
	{
		return NotGrappleable;
	}
	const FVector HookLocation{HookPoint.GetActorLocation()};
	const float DistanceFromOwner{static_cast<float>(
		FVector::Dist(HookLocation,
			CachedOwnerCharacter->GetActorLocation()))};
	if (DistanceFromOwner
			> UBacklotTuningSettings::Get().GrappleRange
		|| DistanceFromOwner
			< GrappleConstants::MinimumGrappleDistance)
	{
		return NotGrappleable;
	}
	if (!HasLineOfSight(ViewLocation, HookLocation))
	{
		return NotGrappleable;
	}
	const FVector DirectionToHook{
		(HookLocation - ViewLocation).GetSafeNormal()};
	const float AimDot{static_cast<float>(
		FVector::DotProduct(ViewDirection, DirectionToHook))};
	return FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(AimDot, -1.0f, 1.0f)));
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
	if (!TargetedHookPoint.IsValid()
		|| !CachedOwnerCharacter.IsValid())
	{
		return false;
	}
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	DestinationHookPoint = TargetedHookPoint;
	GrappleStartLocation =
		CachedOwnerCharacter->GetActorLocation();
	GrappleDestination = TargetedHookPoint->GetActorLocation();
	const float TravelDistance{static_cast<float>(FVector::Dist(
		GrappleStartLocation, GrappleDestination))};
	GrappleDurationSeconds = FMath::Max(
		Tuning.GrappleMinimumTravelSeconds,
		TravelDistance / Tuning.GrappleTravelSpeed);
	GrappleElapsedSeconds = 0.0f;
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
	GrappleElapsedSeconds += DeltaTime;
	const float Progress{FMath::Clamp(
		GrappleElapsedSeconds / GrappleDurationSeconds,
		0.0f, 1.0f)};

	CachedOwnerCharacter->SetActorLocation(
		GetArcLocation(Progress));
	if (Progress >= 1.0f)
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
	bIsGrappling = false;
	DestinationHookPoint.Reset();

	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity =
		TravelDirection * Tuning.GrappleExitForwardSpeed
		+ FVector{0.0f, 0.0f, Tuning.GrappleExitUpwardSpeed};
}

void UGrappleComponent::CancelGrapple()
{
	if (!bIsGrappling || !CachedOwnerCharacter.IsValid())
	{
		return;
	}
	bIsGrappling = false;
	DestinationHookPoint.Reset();
	CachedOwnerCharacter->GetCharacterMovement()
		->SetMovementMode(MOVE_Falling);
}

AGrappleHookPoint* UGrappleComponent::GetTargetedHookPoint() const
{
	return TargetedHookPoint.Get();
}

bool UGrappleComponent::IsGrappling() const
{
	return bIsGrappling;
}

FVector UGrappleComponent::GetGrappleDestination() const
{
	return GrappleDestination;
}
