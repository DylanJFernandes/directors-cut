#include "Game/BacklotGameState.h"

#include "Data/BacklotTuningSettings.h"

ABacklotGameState::ABacklotGameState()
	: bIsControlsHelpVisible{false},
	  UploadProgress{0.0f},
	  bIsUploadActive{false},
	  bIsUploadPaused{false},
	  bIsGameWon{false},
	  TakesRemaining{
		  UBacklotTuningSettings::Get().PlayerTakeCount},
	  bIsGameLost{false},
	  bDrawDebugVisionCones{true}
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABacklotGameState::SetCurrentZoneName(
	const FString& InCurrentZoneName)
{
	CurrentZoneName = InCurrentZoneName;
}

const FString& ABacklotGameState::GetCurrentZoneName() const
{
	return CurrentZoneName;
}

void ABacklotGameState::ShowControlsHelp()
{
	bIsControlsHelpVisible = true;
}

void ABacklotGameState::HideControlsHelp()
{
	bIsControlsHelpVisible = false;
}

bool ABacklotGameState::IsControlsHelpVisible() const
{
	return bIsControlsHelpVisible;
}

void ABacklotGameState::SetUploadProgress(float InUploadProgress)
{
	UploadProgress = FMath::Clamp(InUploadProgress, 0.0f, 1.0f);
}

float ABacklotGameState::GetUploadProgress() const
{
	return UploadProgress;
}

void ABacklotGameState::MarkUploadStarted()
{
	bIsUploadActive = true;
}

void ABacklotGameState::MarkUploadStopped()
{
	bIsUploadActive = false;
	bIsUploadPaused = false;
	UploadProgress = 0.0f;
}

bool ABacklotGameState::IsUploadActive() const
{
	return bIsUploadActive;
}

void ABacklotGameState::SetUploadPaused(bool bInIsUploadPaused)
{
	bIsUploadPaused = bInIsUploadPaused;
}

bool ABacklotGameState::IsUploadPaused() const
{
	return bIsUploadPaused;
}

void ABacklotGameState::MarkGameWon()
{
	bIsGameWon = true;
}

bool ABacklotGameState::IsGameWon() const
{
	return bIsGameWon;
}

bool ABacklotGameState::UseTake()
{
	TakesRemaining = FMath::Max(TakesRemaining - 1, 0);
	return TakesRemaining > 0;
}

int32 ABacklotGameState::GetTakesRemaining() const
{
	return TakesRemaining;
}

void ABacklotGameState::MarkGameLost()
{
	bIsGameLost = true;
}

bool ABacklotGameState::IsGameLost() const
{
	return bIsGameLost;
}

bool ABacklotGameState::IsGameOver() const
{
	return bIsGameWon || bIsGameLost;
}

void ABacklotGameState::ToggleDebugVisionCones()
{
	bDrawDebugVisionCones = !bDrawDebugVisionCones;
}

bool ABacklotGameState::ShouldDrawDebugVisionCones() const
{
	return bDrawDebugVisionCones;
}
