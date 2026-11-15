#include "Component/TakedownComponent.h"

#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Player/DirectorPlayerController.h"
#include "UI/HUD/BacklotHUD.h"

namespace TakedownConstants
{
	/* The director must be at least this far above a guard for
	   the takedown to count as coming from above */
	constexpr float MinimumHeightAboveGuard{150.0f};

	/* Speed at which the director drops onto a guard */
	constexpr float DropOntoGuardSpeed{1400.0f};

	/* Upward part of the knockback from a failed takedown */
	constexpr float KnockbackUpwardSpeed{320.0f};
}

UTakedownComponent::UTakedownComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTakedownComponent::TryTakedown()
{
	const FTakedownOpportunity Opportunity{
		FindTakedownOpportunity()};
	if (!Opportunity.Guard.IsValid())
	{
		return;
	}
	if (Opportunity.bWillSucceed)
	{
		PerformTakedown(Opportunity);
		return;
	}
	FailTakedown(*Opportunity.Guard);
}

FTakedownOpportunity
UTakedownComponent::FindTakedownOpportunity() const
{
	FTakedownOpportunity Opportunity;
	float BestDistance{TNumericLimits<float>::Max()};
	const FVector OwnerLocation{GetOwner()->GetActorLocation()};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		const AGuardCharacter& Guard{**GuardIterator};
		const bool bIsBelow{IsGuardBelowInReach(Guard)};
		const float Distance{static_cast<float>(FVector::Dist(
			OwnerLocation, Guard.GetActorLocation()))};
		const bool bIsInReach{
			bIsBelow || IsGuardInGroundReach(Guard)};
		if (Guard.IsTakenDown() || !bIsInReach
			|| Distance >= BestDistance)
		{
			continue;
		}
		BestDistance = Distance;
		Opportunity.Guard = *GuardIterator;
		Opportunity.bIsFromAbove = bIsBelow;
		Opportunity.bWillSucceed =
			WouldTakedownSucceed(Guard, bIsBelow);
	}
	return Opportunity;
}

bool UTakedownComponent::IsGuardInGroundReach(
	const AGuardCharacter& Guard) const
{
	return FVector::Dist(GetOwner()->GetActorLocation(),
		Guard.GetActorLocation())
		<= UBacklotTuningSettings::Get().TakedownRange;
}

bool UTakedownComponent::IsGuardBelowInReach(
	const AGuardCharacter& Guard) const
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const FVector OwnerLocation{GetOwner()->GetActorLocation()};
	const FVector GuardLocation{Guard.GetActorLocation()};
	const float HeightAboveGuard{
		static_cast<float>(OwnerLocation.Z - GuardLocation.Z)};
	const bool bIsHighEnough{HeightAboveGuard
		>= TakedownConstants::MinimumHeightAboveGuard
		&& HeightAboveGuard <= Tuning.AerialTakedownMaximumDrop};
	const bool bIsCloseEnough{
		FVector::Dist2D(OwnerLocation, GuardLocation)
		<= Tuning.AerialTakedownHorizontalRange};
	if (!bIsHighEnough || !bIsCloseEnough)
	{
		return false;
	}
	FHitResult Hit;
	const bool bIsBlocked{GetWorld()->LineTraceSingleByChannel(
		Hit, OwnerLocation, GuardLocation, ECC_Visibility)};
	return !bIsBlocked;
}

bool UTakedownComponent::WouldTakedownSucceed(
	const AGuardCharacter& Guard, bool bIsFromAbove) const
{
	if (Guard.IsStunned())
	{
		return true;
	}
	if (Guard.IsAlert())
	{
		return false;
	}
	return bIsFromAbove
		|| !Guard.IsFacingLocation(
			GetOwner()->GetActorLocation());
}

void UTakedownComponent::PerformTakedown(
	const FTakedownOpportunity& Opportunity)
{
	AGuardCharacter* Guard{Opportunity.Guard.Get()};
	const FVector GuardLocation{Guard->GetActorLocation()};
	ACharacter* OwnerCharacter{Cast<ACharacter>(GetOwner())};
	if (Opportunity.bIsFromAbove && OwnerCharacter)
	{
		const FVector DropDirection{(GuardLocation
			- OwnerCharacter->GetActorLocation()).GetSafeNormal()};
		OwnerCharacter->LaunchCharacter(DropDirection
			* TakedownConstants::DropOntoGuardSpeed, true, true);
	}
	Guard->ReceiveTakedown();
	ShowPopup(TEXT("CUT!"),
		GuardLocation + FVector{0.0f, 0.0f, 120.0f},
		FLinearColor{1.0f, 0.9f, 0.2f});
}

void UTakedownComponent::FailTakedown(AGuardCharacter& Guard)
{
	const FVector OwnerLocation{GetOwner()->GetActorLocation()};
	Guard.BecomeAlert(OwnerLocation);
	ShowPopup(TEXT("NO GOOD!"),
		Guard.GetActorLocation() + FVector{0.0f, 0.0f, 120.0f},
		FLinearColor{1.0f, 0.2f, 0.2f});

	ACharacter* OwnerCharacter{Cast<ACharacter>(GetOwner())};
	if (!OwnerCharacter)
	{
		return;
	}
	const FVector AwayDirection{(OwnerLocation
		- Guard.GetActorLocation()).GetSafeNormal2D()};
	OwnerCharacter->LaunchCharacter(AwayDirection
		* UBacklotTuningSettings::Get()
			.FailedTakedownKnockbackSpeed
		+ FVector{0.0f, 0.0f,
			TakedownConstants::KnockbackUpwardSpeed},
		true, true);
}

void UTakedownComponent::ShowPopup(const FString& Text,
	const FVector& WorldLocation,
	const FLinearColor& Color) const
{
	const APawn* OwnerPawn{Cast<APawn>(GetOwner())};
	ADirectorPlayerController* PlayerController{OwnerPawn
		? Cast<ADirectorPlayerController>(
			OwnerPawn->GetController())
		: nullptr};
	if (!PlayerController || !PlayerController->GetBacklotHUD())
	{
		return;
	}
	PlayerController->GetBacklotHUD()->ShowWorldPopup(Text,
		WorldLocation, Color);
}
