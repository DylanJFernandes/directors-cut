#include "Game/BacklotGameMode.h"

#include "Actor/GreyboxLevelBuilder.h"
#include "Actor/UploadTerminal.h"
#include "Actor/ZoneCheckpoint.h"
#include "Character/DirectorCharacter.h"
#include "Character/GuardCharacter.h"
#include "EngineUtils.h"
#include "Game/BacklotGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/DirectorPlayerController.h"
#include "TimerManager.h"
#include "UI/HUD/BacklotHUD.h"

ABacklotGameMode::ABacklotGameMode()
	: CheckpointTransform{FTransform::Identity},
	  bIsRestartQueued{false}
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultPawnClass = ADirectorCharacter::StaticClass();
	PlayerControllerClass =
		ADirectorPlayerController::StaticClass();
	HUDClass = ABacklotHUD::StaticClass();
	GameStateClass = ABacklotGameState::StaticClass();
}

void ABacklotGameMode::BeginPlay()
{
	Super::BeginPlay();
	const TActorIterator<AGreyboxLevelBuilder> BuilderIterator{
		GetWorld()};
	if (!BuilderIterator)
	{
		GetWorld()->SpawnActor<AGreyboxLevelBuilder>();
	}
}

void ABacklotGameMode::StartRunAt(
	const AZoneCheckpoint& FirstCheckpoint)
{
	ActivateCheckpoint(FirstCheckpoint);
	if (ADirectorCharacter* Director{GetDirector()})
	{
		Director->RespawnAt(CheckpointTransform);
	}
}

void ABacklotGameMode::ActivateCheckpoint(
	const AZoneCheckpoint& Checkpoint)
{
	CheckpointTransform = Checkpoint.GetActorTransform();
	ABacklotGameState* BacklotGameState{
		GetGameState<ABacklotGameState>()};
	if (!BacklotGameState)
	{
		return;
	}
	BacklotGameState->SetCurrentZoneName(Checkpoint.GetZoneName());
	if (Checkpoint.ShowsControlsHelp())
	{
		BacklotGameState->ShowControlsHelp();
		return;
	}
	BacklotGameState->HideControlsHelp();
}

void ABacklotGameMode::HandlePlayerCaught()
{
	if (bIsRestartQueued)
	{
		return;
	}
	bIsRestartQueued = true;
	GetWorldTimerManager().SetTimerForNextTick(this,
		&ABacklotGameMode::RestartFromCheckpoint);
}

void ABacklotGameMode::RestartFromCheckpoint()
{
	bIsRestartQueued = false;
	if (ADirectorCharacter* Director{GetDirector()})
	{
		Director->RespawnAt(CheckpointTransform);
	}
	ResetUploadTerminals();
	ResetGuards();
	ShowCenterMessage(
		TEXT("CAUGHT!  Back to the last checkpoint."));
}

void ABacklotGameMode::ResetGuards() const
{
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		GuardIterator->ResetToPatrolStart();
	}
}

void ABacklotGameMode::ResetUploadTerminals() const
{
	for (TActorIterator<AUploadTerminal> TerminalIterator{
			GetWorld()};
		TerminalIterator; ++TerminalIterator)
	{
		TerminalIterator->ResetUpload();
	}
}

void ABacklotGameMode::HandleUploadComplete()
{
	if (ABacklotGameState* BacklotGameState{
		GetGameState<ABacklotGameState>()})
	{
		BacklotGameState->MarkGameWon();
	}
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		GuardIterator->ReceiveTakedown();
	}
}

void ABacklotGameMode::RestartRun()
{
	const FString LevelName{
		UGameplayStatics::GetCurrentLevelName(this, true)};
	UGameplayStatics::OpenLevel(this, FName{*LevelName});
}

void ABacklotGameMode::ShowCenterMessage(
	const FString& Message) const
{
	ADirectorPlayerController* PlayerController{
		Cast<ADirectorPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0))};
	if (PlayerController && PlayerController->GetBacklotHUD())
	{
		PlayerController->GetBacklotHUD()->ShowCenterMessage(
			Message);
	}
}

ADirectorCharacter* ABacklotGameMode::GetDirector() const
{
	return Cast<ADirectorCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
}
