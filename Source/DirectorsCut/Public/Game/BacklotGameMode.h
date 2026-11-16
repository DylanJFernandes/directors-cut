#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BacklotGameMode.generated.h"

class ADirectorCharacter;
class AZoneCheckpoint;

/* Rules of the escape: checkpoints, what happens when the
   director is caught, losing by running out of takes, and
   winning by finishing the upload */
UCLASS()
class ABacklotGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/* Sets the pawn, controller, HUD and game state classes */
	ABacklotGameMode();

	/* Makes a checkpoint the current zone and restart spot */
	void ActivateCheckpoint(const AZoneCheckpoint& Checkpoint);

	/* Spends a take and sends the director back to the last
	   checkpoint, or ends the run when no takes are left */
	void HandlePlayerCaught();

	/* Wins the game */
	void HandleUploadComplete();

	/* Reloads the level for a fresh run */
	void RestartRun();

protected:
	/* Makes the map's run start checkpoint the current zone */
	virtual void BeginPlay() override;

private:
	/* Activates the checkpoint marked as the start of the run */
	void ActivateRunStartCheckpoint();

	/* Moves the director to the checkpoint and resets the
	   guards and the upload */
	void RestartFromCheckpoint();

	/* Ends the run as lost and freezes the game behind the
	   game over screen */
	void LoseRun();

	/* Returns every guard to the start of its patrol */
	void ResetGuards() const;

	/* Stops any upload in progress */
	void ResetUploadTerminals() const;

	/* Shows a message in the middle of the screen */
	void ShowCenterMessage(const FString& Message) const;

	/* Returns the director character */
	ADirectorCharacter* GetDirector() const;

	/* Where the director restarts when caught */
	FTransform CheckpointTransform;

	/* Whether a restart from the checkpoint is already queued */
	bool bIsRestartQueued;
};
