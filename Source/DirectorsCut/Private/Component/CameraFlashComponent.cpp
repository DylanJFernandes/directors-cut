#include "Component/CameraFlashComponent.h"

#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

UCameraFlashComponent::UCameraFlashComponent()
	: LastFlashTimeSeconds{-1000.0f}
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCameraFlashComponent::TryFlash()
{
	if (GetCooldownFractionRemaining() > 0.0f)
	{
		return false;
	}
	LastFlashTimeSeconds = GetWorld()->GetTimeSeconds();
	StunGuardsInCone();
	return true;
}

int32 UCameraFlashComponent::StunGuardsInCone() const
{
	const APawn* OwnerPawn{Cast<APawn>(GetOwner())};
	if (!OwnerPawn)
	{
		return 0;
	}
	const FVector FlashOrigin{OwnerPawn->GetPawnViewLocation()};
	const FVector FlashDirection{
		OwnerPawn->GetControlRotation().Vector()};
	const float StunSeconds{
		UBacklotTuningSettings::Get().FlashStunSeconds};

	int32 StunnedGuardCount{0};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		if (IsGuardCaughtInFlash(**GuardIterator, FlashOrigin,
			FlashDirection))
		{
			GuardIterator->ApplyStun(StunSeconds);
			++StunnedGuardCount;
		}
	}
	return StunnedGuardCount;
}

bool UCameraFlashComponent::IsGuardCaughtInFlash(
	const AGuardCharacter& Guard, const FVector& FlashOrigin,
	const FVector& FlashDirection) const
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const FVector GuardLocation{Guard.GetActorLocation()};
	const FVector ToGuard{GuardLocation - FlashOrigin};
	if (Guard.IsTakenDown() || ToGuard.Size() > Tuning.FlashRange)
	{
		return false;
	}
	const float MinimumDot{FMath::Cos(FMath::DegreesToRadians(
		Tuning.FlashConeHalfAngleDegrees))};
	if (FVector::DotProduct(FlashDirection,
		ToGuard.GetSafeNormal()) < MinimumDot)
	{
		return false;
	}
	FHitResult Hit;
	const bool bIsBehindWall{
		GetWorld()->LineTraceSingleByChannel(Hit, FlashOrigin,
			GuardLocation, ECC_Visibility)};
	return !bIsBehindWall;
}

float UCameraFlashComponent::GetCooldownFractionRemaining() const
{
	const float CooldownSeconds{
		UBacklotTuningSettings::Get().FlashCooldownSeconds};
	return FMath::Clamp(
		1.0f - GetSecondsSinceLastFlash() / CooldownSeconds,
		0.0f, 1.0f);
}

float UCameraFlashComponent::GetSecondsSinceLastFlash() const
{
	return GetWorld()->GetTimeSeconds() - LastFlashTimeSeconds;
}
