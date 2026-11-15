#include "Component/DirectorLocomotionComponent.h"

#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDirectorLocomotionComponent::UDirectorLocomotionComponent()
	: bWantsToSprint{false},
	  bIsSliding{false},
	  bIsSlideQueued{false},
	  SlideChainCount{0},
	  LastSlideEndTimeSeconds{-1000.0f},
	  DefaultGroundFriction{8.0f},
	  DefaultBrakingDeceleration{2048.0f}
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDirectorLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedOwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!CachedOwnerCharacter.IsValid())
	{
		return;
	}
	const UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	DefaultGroundFriction = Movement->GroundFriction;
	DefaultBrakingDeceleration =
		Movement->BrakingDecelerationWalking;
}

void UDirectorLocomotionComponent::TickComponent(float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!CachedOwnerCharacter.IsValid())
	{
		return;
	}
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	Movement->MaxWalkSpeed = IsSprinting()
		? Tuning.PlayerSprintSpeed : Tuning.PlayerWalkSpeed;

	const bool bSlideRanOut{bIsSliding
		&& Movement->IsMovingOnGround()
		&& Movement->Velocity.Size2D() <= Tuning.SlideEndSpeed};
	if (bSlideRanOut)
	{
		EndSlide(bWantsToSprint);
	}
}

void UDirectorLocomotionComponent::SetWantsToSprint(
	bool bInWantsToSprint)
{
	bWantsToSprint = bInWantsToSprint;
}

void UDirectorLocomotionComponent::HandleCrouchPressed()
{
	if (!CachedOwnerCharacter.IsValid())
	{
		return;
	}
	if (bIsSliding)
	{
		EndSlide(false);
		return;
	}
	if (CachedOwnerCharacter->IsCrouched())
	{
		CachedOwnerCharacter->UnCrouch();
		return;
	}
	if (!HasSlideEntrySpeed())
	{
		CachedOwnerCharacter->Crouch();
		return;
	}
	if (CachedOwnerCharacter->GetCharacterMovement()
		->IsMovingOnGround())
	{
		StartSlide();
		return;
	}
	bIsSlideQueued = true;
}

bool UDirectorLocomotionComponent::TrySlideJump()
{
	if (!bIsSliding || !CachedOwnerCharacter.IsValid())
	{
		return false;
	}
	EndSlide(true);
	CachedOwnerCharacter->LaunchCharacter(FVector{0.0f, 0.0f,
		UBacklotTuningSettings::Get().PlayerJumpVelocity},
		false, true);
	return true;
}

void UDirectorLocomotionComponent::HandleLanded()
{
	if (!bIsSlideQueued || !CachedOwnerCharacter.IsValid())
	{
		return;
	}
	bIsSlideQueued = false;
	if (HasSlideEntrySpeed())
	{
		StartSlide();
		return;
	}
	CachedOwnerCharacter->Crouch();
}

void UDirectorLocomotionComponent::HandleWalkedOffLedge()
{
	if (!bIsSliding || !CachedOwnerCharacter.IsValid())
	{
		return;
	}
	EndSlide(true);
	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	Movement->Velocity += Movement->Velocity.GetSafeNormal2D()
		* UBacklotTuningSettings::Get().SlideEdgeBoostSpeed;
}

void UDirectorLocomotionComponent::CancelSlide()
{
	bIsSlideQueued = false;
	if (bIsSliding)
	{
		EndSlide(true);
	}
}

bool UDirectorLocomotionComponent::HasSlideEntrySpeed() const
{
	return CachedOwnerCharacter->GetVelocity().Size2D()
		>= UBacklotTuningSettings::Get().SlideMinimumEntrySpeed;
}

void UDirectorLocomotionComponent::StartSlide()
{
	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	FVector SlideDirection{Movement->Velocity.GetSafeNormal2D()};
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection =
			CachedOwnerCharacter->GetActorForwardVector();
	}
	bIsSliding = true;
	bIsSlideQueued = false;
	Movement->GroundFriction = 0.0f;
	Movement->BrakingDecelerationWalking =
		UBacklotTuningSettings::Get().SlideDeceleration;
	Movement->Velocity = SlideDirection * GetNextSlideStartSpeed()
		+ FVector{0.0f, 0.0f, Movement->Velocity.Z};
	CachedOwnerCharacter->Crouch();
}

float UDirectorLocomotionComponent::GetNextSlideStartSpeed()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const float SecondsSinceLastSlide{static_cast<float>(
		GetWorld()->GetTimeSeconds() - LastSlideEndTimeSeconds)};
	if (SecondsSinceLastSlide > Tuning.SlideChainResetSeconds)
	{
		SlideChainCount = 0;
	}
	const float ChainedSpeed{Tuning.SlideStartSpeed * FMath::Pow(
		Tuning.SlideChainSpeedMultiplier,
		static_cast<float>(SlideChainCount))};
	++SlideChainCount;
	return FMath::Max(ChainedSpeed, Tuning.PlayerSprintSpeed);
}

void UDirectorLocomotionComponent::EndSlide(bool bShouldStandUp)
{
	bIsSliding = false;
	LastSlideEndTimeSeconds = GetWorld()->GetTimeSeconds();
	if (!CachedOwnerCharacter.IsValid())
	{
		return;
	}
	UCharacterMovementComponent* Movement{
		CachedOwnerCharacter->GetCharacterMovement()};
	Movement->GroundFriction = DefaultGroundFriction;
	Movement->BrakingDecelerationWalking =
		DefaultBrakingDeceleration;
	if (bShouldStandUp)
	{
		CachedOwnerCharacter->UnCrouch();
	}
}

bool UDirectorLocomotionComponent::IsSprinting() const
{
	return bWantsToSprint && !bIsSliding
		&& CachedOwnerCharacter.IsValid()
		&& !CachedOwnerCharacter->IsCrouched();
}

bool UDirectorLocomotionComponent::IsSliding() const
{
	return bIsSliding;
}

float UDirectorLocomotionComponent::GetMoveInputScale() const
{
	return bIsSliding
		? UBacklotTuningSettings::Get().SlideSteeringFraction
		: 1.0f;
}
