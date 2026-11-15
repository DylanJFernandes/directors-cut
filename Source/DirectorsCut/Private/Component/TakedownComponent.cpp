#include "Component/TakedownComponent.h"

#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "EngineUtils.h"
#include "Game/BacklotSoundLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/DirectorPlayerController.h"
#include "TimerManager.h"
#include "UI/HUD/BacklotHUD.h"

namespace TakedownConstants
{
	/* The director must be at least this far above a guard for
	   the takedown to count as coming from above */
	constexpr float MinimumHeightAboveGuard{150.0f};

	/* Speed at which the director drops onto a guard */
	constexpr float DropOntoGuardSpeed{1400.0f};

	/* Extra seconds the held guard stays frozen, so he cannot
	   break free on the frame the takedown lands */
	constexpr float StruggleHoldMarginSeconds{0.5f};
}

UTakedownComponent::UTakedownComponent()
	: StruggleStartTimeSeconds{0.0f}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTakedownComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	EndDramaticCut();
	Super::EndPlay(EndPlayReason);
}

void UTakedownComponent::TryTakedown()
{
	const FTakedownOpportunity Opportunity{
		FindTakedownOpportunity()};
	if (!Opportunity.Guard.IsValid() || IsStruggling())
	{
		return;
	}
	if (Opportunity.bIsInstant)
	{
		PerformTakedown(Opportunity);
		return;
	}
	BeginStruggle(*Opportunity.Guard);
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
		const bool bIsBelow{!Guard.IsAlert()
			&& IsGuardBelowInReach(Guard)};
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
		Opportunity.bIsInstant =
			IsGuardCaughtOffGuard(Guard, bIsBelow);
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

bool UTakedownComponent::IsGuardCaughtOffGuard(
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
	PlayCutFeedback(GuardLocation);
}

void UTakedownComponent::PlayCutFeedback(
	const FVector& CutLocation)
{
	BacklotSoundLibrary::PlayClapperboard(this, CutLocation);
	ShowPopup(TEXT("CUT!"),
		CutLocation + FVector{0.0f, 0.0f, 120.0f},
		FLinearColor{1.0f, 0.9f, 0.2f});
	StartDramaticCut();
}

void UTakedownComponent::StartDramaticCut()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const float TimeDilation{FMath::Clamp(
		Tuning.TakedownSlowMotionTimeDilation, 0.05f, 1.0f)};
	UGameplayStatics::SetGlobalTimeDilation(this, TimeDilation);
	const float DilatedSeconds{FMath::Max(0.01f,
		Tuning.TakedownSlowMotionSeconds * TimeDilation)};
	GetWorld()->GetTimerManager().SetTimer(
		DramaticCutTimerHandle, this,
		&UTakedownComponent::EndDramaticCut, DilatedSeconds);
}

void UTakedownComponent::EndDramaticCut()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
}

void UTakedownComponent::BeginStruggle(AGuardCharacter& Guard)
{
	const float StruggleSeconds{
		UBacklotTuningSettings::Get().AlertTakedownSeconds};
	const FVector OwnerLocation{GetOwner()->GetActorLocation()};
	StrugglingGuard = &Guard;
	StruggleStartTimeSeconds = GetWorld()->GetTimeSeconds();
	Guard.BecomeAlert(OwnerLocation);
	Guard.ApplyStun(StruggleSeconds
		+ TakedownConstants::StruggleHoldMarginSeconds);

	if (ACharacter* OwnerCharacter{Cast<ACharacter>(GetOwner())})
	{
		const FVector ToGuard{
			Guard.GetActorLocation() - OwnerLocation};
		OwnerCharacter->SetActorRotation(
			FRotator{0.0f, ToGuard.Rotation().Yaw, 0.0f});
		OwnerCharacter->GetCharacterMovement()
			->StopMovementImmediately();
		OwnerCharacter->GetCharacterMovement()->DisableMovement();
	}
	GetWorld()->GetTimerManager().SetTimer(StruggleTimerHandle,
		this, &UTakedownComponent::FinishStruggle,
		StruggleSeconds);
}

void UTakedownComponent::FinishStruggle()
{
	AGuardCharacter* Guard{StrugglingGuard.Get()};
	StrugglingGuard.Reset();
	ReleaseOwnerMovement();
	if (!Guard || Guard->IsTakenDown())
	{
		return;
	}
	FTakedownOpportunity Opportunity;
	Opportunity.Guard = Guard;
	Opportunity.bIsInstant = true;
	PerformTakedown(Opportunity);
}

void UTakedownComponent::CancelStruggle()
{
	if (!IsStruggling())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(StruggleTimerHandle);
	StrugglingGuard.Reset();
	ReleaseOwnerMovement();
}

void UTakedownComponent::ReleaseOwnerMovement() const
{
	const ACharacter* OwnerCharacter{
		Cast<ACharacter>(GetOwner())};
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement()
		->MovementMode == MOVE_None)
	{
		OwnerCharacter->GetCharacterMovement()
			->SetMovementMode(MOVE_Walking);
	}
}

bool UTakedownComponent::IsStruggling() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(
		StruggleTimerHandle);
}

float UTakedownComponent::GetStruggleProgress() const
{
	if (!IsStruggling())
	{
		return 0.0f;
	}
	const float ElapsedSeconds{static_cast<float>(
		GetWorld()->GetTimeSeconds() - StruggleStartTimeSeconds)};
	return FMath::Clamp(ElapsedSeconds
		/ UBacklotTuningSettings::Get().AlertTakedownSeconds,
		0.0f, 1.0f);
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
