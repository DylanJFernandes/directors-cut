#include "Actor/CameraRigTurret.h"

#include "Character/GuardCharacter.h"
#include "Component/BeamMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/GreyboxShapeLibrary.h"
#include "TimerManager.h"

namespace CameraRigConstants
{
	/* Distance inside which E mounts the rig */
	constexpr float MountRange{260.0f};

	/* Where the director stands, relative to the rig */
	const FVector SeatOffset{-90.0f, 0.0f, 96.0f};

	/* Where the spotlight beam starts, relative to the rig */
	const FVector LensOffset{0.0f, 0.0f, 190.0f};

	/* Seconds the spotlight beam stays visible */
	constexpr float BeamFlashSeconds{0.12f};

	/* Thickness of the spotlight beam */
	constexpr float BeamThickness{14.0f};
}

ACameraRigTurret::ACameraRigTurret()
	: LastFireTimeSeconds{-1000.0f}
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	BaseMesh->SetRelativeScale3D(FVector{0.5f, 0.5f, 1.5f});

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(BaseMesh);
	HeadMesh->SetUsingAbsoluteScale(true);
	HeadMesh->SetRelativeLocation(FVector{0.0f, 0.0f, 75.0f});
	HeadMesh->SetWorldScale3D(FVector{1.1f, 0.5f, 0.5f});
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpotlightBeam = CreateDefaultSubobject<UBeamMeshComponent>(
		TEXT("SpotlightBeam"));
	SpotlightBeam->SetupAttachment(BaseMesh);
}

void ACameraRigTurret::BeginPlay()
{
	Super::BeginPlay();
	BaseMesh->SetStaticMesh(
		GreyboxShapeLibrary::GetCylinderMesh());
	GreyboxShapeLibrary::ApplyFlatColor(BaseMesh,
		FLinearColor{0.05f, 0.05f, 0.06f});
	HeadMesh->SetStaticMesh(GreyboxShapeLibrary::GetCubeMesh());
	GreyboxShapeLibrary::ApplyFlatColor(HeadMesh,
		FLinearColor{0.8f, 0.6f, 0.05f});
	SpotlightBeam->SetBeamThickness(
		CameraRigConstants::BeamThickness);
	SpotlightBeam->SetBeamColor(FLinearColor{1.0f, 1.0f, 0.8f});
}

bool ACameraRigTurret::CanBeMountedFrom(
	const FVector& UserLocation) const
{
	return FVector::Dist2D(GetActorLocation(), UserLocation)
		<= CameraRigConstants::MountRange;
}

FVector ACameraRigTurret::GetSeatLocation() const
{
	return GetActorLocation() - FVector{0.0f, 0.0f,
		BaseMesh->Bounds.BoxExtent.Z}
		+ CameraRigConstants::SeatOffset;
}

bool ACameraRigTurret::TryFireSpotlight(
	const FVector& ViewLocation, const FVector& ViewDirection)
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const float CurrentTimeSeconds{
		static_cast<float>(GetWorld()->GetTimeSeconds())};
	if (CurrentTimeSeconds - LastFireTimeSeconds
		< Tuning.TurretFireIntervalSeconds)
	{
		return false;
	}
	LastFireTimeSeconds = CurrentTimeSeconds;

	AGuardCharacter* AimedGuard{
		FindAimedGuard(ViewLocation, ViewDirection)};
	const FVector BeamEnd{AimedGuard
		? AimedGuard->GetActorLocation()
		: ViewLocation + ViewDirection * Tuning.TurretRange};
	if (AimedGuard)
	{
		AimedGuard->ApplyStun(Tuning.TurretStunSeconds);
	}
	SpotlightBeam->ShowBeamBetween(GetLensLocation(), BeamEnd);
	GetWorldTimerManager().SetTimer(BeamHideTimerHandle, this,
		&ACameraRigTurret::HideSpotlightBeam,
		CameraRigConstants::BeamFlashSeconds);
	return true;
}

AGuardCharacter* ACameraRigTurret::FindAimedGuard(
	const FVector& ViewLocation,
	const FVector& ViewDirection) const
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	AGuardCharacter* BestGuard{nullptr};
	float BestAimDot{FMath::Cos(FMath::DegreesToRadians(
		Tuning.TurretAimConeHalfAngleDegrees))};
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		const FVector GuardLocation{
			GuardIterator->GetActorLocation()};
		const float AimDot{static_cast<float>(FVector::DotProduct(
			ViewDirection,
			(GuardLocation - ViewLocation).GetSafeNormal()))};
		const bool bIsInRange{FVector::Dist(GetActorLocation(),
			GuardLocation) <= Tuning.TurretRange};
		FHitResult Hit;
		if (!GuardIterator->IsTakenDown() && bIsInRange
			&& AimDot > BestAimDot
			&& !GetWorld()->LineTraceSingleByChannel(Hit,
				GetLensLocation(), GuardLocation, ECC_Visibility))
		{
			BestAimDot = AimDot;
			BestGuard = *GuardIterator;
		}
	}
	return BestGuard;
}

FVector ACameraRigTurret::GetLensLocation() const
{
	return GetActorLocation() - FVector{0.0f, 0.0f,
		BaseMesh->Bounds.BoxExtent.Z}
		+ CameraRigConstants::LensOffset;
}

void ACameraRigTurret::HideSpotlightBeam()
{
	SpotlightBeam->HideBeam();
}
