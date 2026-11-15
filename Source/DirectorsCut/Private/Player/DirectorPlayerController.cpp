#include "Player/DirectorPlayerController.h"

#include "Components/InputComponent.h"
#include "Game/BacklotGameMode.h"
#include "Game/BacklotGameState.h"
#include "UI/HUD/BacklotHUD.h"

void ADirectorPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly{});
	bShowMouseCursor = false;
}

void ADirectorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::R, IE_Pressed, this,
		&ADirectorPlayerController::HandleRestartPressed);
	InputComponent->BindKey(EKeys::F1, IE_Pressed, this,
		&ADirectorPlayerController
			::HandleToggleVisionConesPressed);
}

void ADirectorPlayerController::HandleRestartPressed()
{
	const ABacklotGameState* GameState{
		GetWorld()->GetGameState<ABacklotGameState>()};
	ABacklotGameMode* GameMode{
		GetWorld()->GetAuthGameMode<ABacklotGameMode>()};
	if (GameState && GameMode && GameState->IsGameWon())
	{
		GameMode->RestartRun();
	}
}

void ADirectorPlayerController::HandleToggleVisionConesPressed()
{
	ABacklotGameState* GameState{
		GetWorld()->GetGameState<ABacklotGameState>()};
	if (GameState)
	{
		GameState->ToggleDebugVisionCones();
	}
}

ABacklotHUD* ADirectorPlayerController::GetBacklotHUD()
{
	if (!CachedBacklotHUD.IsValid())
	{
		CachedBacklotHUD = GetHUD<ABacklotHUD>();
	}
	return CachedBacklotHUD.Get();
}
