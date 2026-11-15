#include "Component/TommyGunComponent.h"

#include "Character/GuardCharacter.h"
#include "Component/BeamMeshComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/BacklotSoundLibrary.h"
#include "Game/TommyGunShape.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace TommyGunConstants
{
	/* Seconds each tracer stays visible */
	constexpr float TracerSeconds{0.04f};

	/* Thickness of the tracer line */
	constexpr float TracerThickness{4.0f};

	/* Half the height of the part of a guard a shot can hit */
	constexpr float GuardHitHalfHeight{75.0f};
}

UTommyGunComponent::UTommyGunComponent()
	: bIsEquipped{false},
	  bIsFiring{false},
	  SecondsUntilNextShot{0.0f},
	  LastHitTimeSeconds{-1000.0f}
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTommyGunComponent::BeginPlay()
{
	Super::BeginPlay();
	Tracer = NewObject<UBeamMeshComponent>(GetOwner());
	Tracer->SetupAttachment(this);
	Tracer->RegisterComponent();
	Tracer->SetBeamThickness(TommyGunConstants::TracerThickness);
	Tracer->SetBeamColor(FLinearColor{1.0f, 0.9f, 0.4f});
}

void UTommyGunComponent::TickComponent(float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SecondsUntilNextShot -= DeltaTime;
	if (!bIsEquipped || !bIsFiring || SecondsUntilNextShot > 0.0f)
	{
		return;
	}
	SecondsUntilNextShot = UBacklotTuningSettings::Get()
		.TommyGunFireIntervalSeconds;
	FireShot();
}

void UTommyGunComponent::FireShot()
{
	const APawn* OwnerPawn{Cast<APawn>(GetOwner())};
	if (!OwnerPawn || !OwnerPawn->GetController())
	{
		return;
	}
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	FVector ViewLocation;
	FRotator ViewRotation;
	OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation,
		ViewRotation);
	const FVector ViewDirection{ViewRotation.Vector()};
	const FVector ShotDirection{FMath::VRandCone(ViewDirection,
		FMath::DegreesToRadians(
			Tuning.TommyGunSpreadHalfAngleDegrees))};
	const FVector ShotStart{ViewLocation + ViewDirection
		* FVector::Dist(ViewLocation,
			OwnerPawn->GetActorLocation())};
	FVector ShotEnd{
		ShotStart + ShotDirection * Tuning.TommyGunRange};

	FCollisionQueryParams QueryParams{
		TEXT("TommyGunShot"), false, GetOwner()};
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ShotStart,
		ShotEnd, ECC_Visibility, QueryParams))
	{
		ShotEnd = Hit.ImpactPoint;
	}
	if (AGuardCharacter* Guard{
		FindGuardOnShotLine(ShotStart, ShotEnd)})
	{
		LastHitTimeSeconds = GetWorld()->GetTimeSeconds();
		const bool bWasKnockedOut{Guard->ReceiveBulletHit(
			OwnerPawn->GetActorLocation(), ShotDirection)};
		if (bWasKnockedOut)
		{
			BacklotSoundLibrary::PlayClapperboard(this, ShotEnd);
		}
	}

	const FVector MuzzleLocation{GetComponentLocation()
		+ GetForwardVector() * TommyGunShape::MuzzleDistance};
	Tracer->ShowBeamBetween(MuzzleLocation, ShotEnd);
	GetWorld()->GetTimerManager().SetTimer(TracerTimerHandle,
		this, &UTommyGunComponent::HideTracer,
		TommyGunConstants::TracerSeconds);
	BacklotSoundLibrary::PlayTommyGunShot(this, MuzzleLocation);
}

AGuardCharacter* UTommyGunComponent::FindGuardOnShotLine(
	const FVector& ShotStart, FVector& InOutShotEnd) const
{
	const float HitRadius{
		UBacklotTuningSettings::Get().TommyGunHitRadius};
	const FVector BodyHalfHeight{0.0f, 0.0f,
		TommyGunConstants::GuardHitHalfHeight};
	AGuardCharacter* NearestGuard{nullptr};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		if (GuardIterator->IsTakenDown())
		{
			continue;
		}
		const FVector GuardLocation{
			GuardIterator->GetActorLocation()};
		FVector PointOnShot;
		FVector PointOnGuard;
		FMath::SegmentDistToSegment(ShotStart, InOutShotEnd,
			GuardLocation - BodyHalfHeight,
			GuardLocation + BodyHalfHeight,
			PointOnShot, PointOnGuard);
		if (FVector::Dist(PointOnShot, PointOnGuard) <= HitRadius)
		{
			InOutShotEnd = PointOnShot;
			NearestGuard = *GuardIterator;
		}
	}
	return NearestGuard;
}

void UTommyGunComponent::HideTracer()
{
	Tracer->HideBeam();
}

void UTommyGunComponent::Equip()
{
	if (bIsEquipped)
	{
		return;
	}
	bIsEquipped = true;
	TommyGunShape::Build(*this);
}

void UTommyGunComponent::StartFiring()
{
	bIsFiring = true;
}

void UTommyGunComponent::StopFiring()
{
	bIsFiring = false;
}

bool UTommyGunComponent::IsEquipped() const
{
	return bIsEquipped;
}

float UTommyGunComponent::GetSecondsSinceLastHit() const
{
	return static_cast<float>(
		GetWorld()->GetTimeSeconds() - LastHitTimeSeconds);
}