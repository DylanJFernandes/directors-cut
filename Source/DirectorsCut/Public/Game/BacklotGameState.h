#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BacklotGameState.generated.h"

/* Shared state of the escape: the zone the player is in, how
   far the upload has come, the takes left, and whether the
   game is won or lost */
UCLASS()
class ABacklotGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/* Sets the starting state of a fresh run */
	ABacklotGameState();

	/* Records the zone the player has entered */
	void SetCurrentZoneName(const FString& InCurrentZoneName);

	/* Returns the name of the zone the player is in */
	const FString& GetCurrentZoneName() const;

	/* Makes the HUD list the controls */
	void ShowControlsHelp();

	/* Stops the HUD listing the controls */
	void HideControlsHelp();

	/* Returns whether the HUD should list the controls */
	bool IsControlsHelpVisible() const;

	/* Records how far the upload has come, from 0 to 1 */
	void SetUploadProgress(float InUploadProgress);

	/* Returns how far the upload has come, from 0 to 1 */
	float GetUploadProgress() const;

	/* Records that the upload has started */
	void MarkUploadStarted();

	/* Records that the upload was reset to not started */
	void MarkUploadStopped();

	/* Returns whether the upload is running */
	bool IsUploadActive() const;

	/* Records whether the player is close enough for the
	   upload to advance */
	void SetUploadPaused(bool bInIsUploadPaused);

	/* Returns whether the upload is paused by distance */
	bool IsUploadPaused() const;

	/* Records that the film was uploaded */
	void MarkGameWon();

	/* Returns whether the film was uploaded */
	bool IsGameWon() const;

	/* Spends one take. Returns whether any takes are left. */
	bool UseTake();

	/* Returns the takes the director has left */
	int32 GetTakesRemaining() const;

	/* Records that the director ran out of takes */
	void MarkGameLost();

	/* Returns whether the director ran out of takes */
	bool IsGameLost() const;

	/* Returns whether the run has ended, won or lost */
	bool IsGameOver() const;

	/* Switches the debug vision cones on or off */
	void ToggleDebugVisionCones();

	/* Returns whether guards should draw their vision cones */
	bool ShouldDrawDebugVisionCones() const;

private:
	/* Name of the zone the player is in */
	FString CurrentZoneName;

	/* Whether the HUD lists the controls */
	bool bIsControlsHelpVisible;

	/* Upload progress from 0 to 1 */
	float UploadProgress;

	/* Whether the upload is running */
	bool bIsUploadActive;

	/* Whether the upload is paused by distance */
	bool bIsUploadPaused;

	/* Whether the film was uploaded */
	bool bIsGameWon;

	/* Takes the director has left */
	int32 TakesRemaining;

	/* Whether the director ran out of takes */
	bool bIsGameLost;

	/* Whether guards draw their vision cones */
	bool bDrawDebugVisionCones;
};
