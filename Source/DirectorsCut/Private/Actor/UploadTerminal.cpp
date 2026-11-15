#include "Actor/UploadTerminal.h"

#include "Character/GuardCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/World.h"
#include "Game/BacklotGameMode.h"
#include "Game/BacklotGameState.h"
#include "Game/GreyboxShapeLibrary.h"
#include "Kismet/GameplayStatics.h"

AUploadTerminal::AUploadTerminal()
	: bIsUploading{false},
	  bIsComplete{false},
	  UploadProgress{0.0f},
	  SecondsUntilNextWave{0.0f},
	  SpawnedWaveCount{0}
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	DeskMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("DeskMesh"));
	SetRootComponent(DeskMesh);
	DeskMesh->SetRelativeScale3D(FVector{0.8f, 2.0f, 1.0f});

	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(DeskMesh);
	ScreenMesh->SetRelativeLocation(FVector{0.0f, 0.0f, 100.0f});
	ScreenMesh->SetRelativeScale3D(FVector{0.15f, 0.8f, 0.9f});
	ScreenMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UTextRenderComponent>(
		TEXT("Label"));
	Label->SetupAttachment(DeskMesh);
	Label->SetUsingAbsoluteScale(true);
	Label->SetRelativeLocation(FVector{0.0f, 0.0f, 230.0f});
	Label->SetRelativeRotation(FRotator{0.0f, 180.0f, 0.0f});
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(60.0f);
	Label->SetText(FText::FromString(TEXT("UPLOAD TERMINAL")));
}

void AUploadTerminal::BeginPlay()
{
	Super::BeginPlay();
	DeskMesh->SetStaticMesh(GreyboxShapeLibrary::GetCubeMesh());
	GreyboxShapeLibrary::ApplyFlatColor(DeskMesh,
		FLinearColor{0.15f, 0.15f, 0.2f});
	ScreenMesh->SetStaticMesh(GreyboxShapeLibrary::GetCubeMesh());
	GreyboxShapeLibrary::ApplyGlowColor(ScreenMesh,
		FLinearColor{0.1f, 1.0f, 0.4f});
}

void AUploadTerminal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceUpload(DeltaSeconds);
	if (bIsUploading)
	{
		UpdateWaves(DeltaSeconds);
	}
}

void AUploadTerminal::AdvanceUpload(float DeltaSeconds)
{
	ABacklotGameState* GameState{GetBacklotGameState()};
	const bool bIsInRange{IsPlayerInUploadRange()};
	if (bIsInRange)
	{
		UploadProgress += DeltaSeconds
			/ UBacklotTuningSettings::Get().UploadDurationSeconds;
	}
	if (GameState)
	{
		GameState->SetUploadPaused(!bIsInRange);
		GameState->SetUploadProgress(UploadProgress);
	}
	if (UploadProgress >= 1.0f)
	{
		CompleteUpload();
	}
}

bool AUploadTerminal::IsPlayerInUploadRange() const
{
	const APawn* PlayerPawn{
		UGameplayStatics::GetPlayerPawn(this, 0)};
	return PlayerPawn && FVector::Dist(GetActorLocation(),
		PlayerPawn->GetActorLocation())
		<= UBacklotTuningSettings::Get()
			.UploadMaximumPlayerDistance;
}

void AUploadTerminal::UpdateWaves(float DeltaSeconds)
{
	SecondsUntilNextWave -= DeltaSeconds;
	if (SecondsUntilNextWave > 0.0f)
	{
		return;
	}
	SecondsUntilNextWave =
		UBacklotTuningSettings::Get().UploadWaveIntervalSeconds;
	SpawnWave();
}

void AUploadTerminal::SpawnWave()
{
	if (GuardSpawnLocations.IsEmpty())
	{
		return;
	}
	const int32 GuardCount{GetNextWaveGuardCount()};
	for (int32 GuardIndex{0}; GuardIndex < GuardCount;
		++GuardIndex)
	{
		const int32 DoorIndex{(SpawnedWaveCount + GuardIndex)
			% GuardSpawnLocations.Num()};
		SpawnWaveGuard(GuardSpawnLocations[DoorIndex]);
	}
	++SpawnedWaveCount;
}

int32 AUploadTerminal::GetNextWaveGuardCount() const
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	const int32 WaveSize{FMath::Min(
		Tuning.UploadFirstWaveGuardCount
			+ Tuning.UploadWaveGuardGrowth * SpawnedWaveCount,
		Tuning.UploadMaximumWaveGuardCount)};
	const int32 RoomLeft{Tuning.UploadMaximumLivingGuards
		- CountLivingWaveGuards()};
	return FMath::Clamp(WaveSize, 0, FMath::Max(RoomLeft, 0));
}

void AUploadTerminal::SpawnWaveGuard(const FVector& SpawnLocation)
{
	const float SpawnSpread{90.0f};
	const FVector SpreadLocation{SpawnLocation + FVector{
		FMath::FRandRange(-SpawnSpread, SpawnSpread),
		FMath::FRandRange(-SpawnSpread, SpawnSpread), 0.0f}};
	const FRotator FacingTerminal{
		(GetActorLocation() - SpreadLocation).Rotation()};
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod
			::AdjustIfPossibleButAlwaysSpawn;
	AGuardCharacter* Guard{
		GetWorld()->SpawnActor<AGuardCharacter>(SpreadLocation,
			FRotator{0.0f, FacingTerminal.Yaw, 0.0f},
			SpawnParameters)};
	if (!Guard)
	{
		return;
	}
	Guard->MakeRelentless();
	Guard->BecomeAlert(GetActorLocation());
	WaveGuards.Add(Guard);
}

int32 AUploadTerminal::CountLivingWaveGuards() const
{
	int32 LivingGuardCount{0};
	for (const TWeakObjectPtr<AGuardCharacter>& Guard : WaveGuards)
	{
		if (Guard.IsValid() && !Guard->IsTakenDown())
		{
			++LivingGuardCount;
		}
	}
	return LivingGuardCount;
}

void AUploadTerminal::CompleteUpload()
{
	bIsUploading = false;
	bIsComplete = true;
	SetActorTickEnabled(false);
	Label->SetText(FText::FromString(TEXT("UPLOAD COMPLETE")));
	ABacklotGameMode* GameMode{
		GetWorld()->GetAuthGameMode<ABacklotGameMode>()};
	if (GameMode)
	{
		GameMode->HandleUploadComplete();
	}
}

void AUploadTerminal::SetGuardSpawnLocations(
	const TArray<FVector>& InGuardSpawnLocations)
{
	GuardSpawnLocations = InGuardSpawnLocations;
}

bool AUploadTerminal::CanBeUsedFrom(
	const FVector& UserLocation) const
{
	const bool bIsInReach{
		FVector::Dist(GetActorLocation(), UserLocation)
		<= UBacklotTuningSettings::Get().UploadInteractionRange};
	return bIsInReach && !bIsUploading && !bIsComplete;
}

void AUploadTerminal::StartUpload()
{
	bIsUploading = true;
	UploadProgress = 0.0f;
	SpawnedWaveCount = 0;
	SecondsUntilNextWave =
		UBacklotTuningSettings::Get().UploadFirstWaveDelaySeconds;
	SetActorTickEnabled(true);
	Label->SetText(FText::FromString(TEXT("UPLOADING...")));
	if (ABacklotGameState* GameState{GetBacklotGameState()})
	{
		GameState->MarkUploadStarted();
	}
}

void AUploadTerminal::ResetUpload()
{
	if (bIsComplete)
	{
		return;
	}
	bIsUploading = false;
	UploadProgress = 0.0f;
	SetActorTickEnabled(false);
	Label->SetText(FText::FromString(TEXT("UPLOAD TERMINAL")));
	for (const TWeakObjectPtr<AGuardCharacter>& Guard : WaveGuards)
	{
		if (Guard.IsValid())
		{
			Guard->Destroy();
		}
	}
	WaveGuards.Reset();
	if (ABacklotGameState* GameState{GetBacklotGameState()})
	{
		GameState->MarkUploadStopped();
	}
}

ABacklotGameState* AUploadTerminal::GetBacklotGameState() const
{
	return GetWorld()->GetGameState<ABacklotGameState>();
}
