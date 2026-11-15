#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BacklotGameMode.generated.h"

class ADirectorCharacter;
class AZoneCheckpoint;

/* Rules of the escape: checkpoints, what happens when the
   director is caught, and winning by finishing the upload */
UCLASS()
class ABacklotGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/* Sets the pawn, controller, HUD and game state classes */
	ABacklotGameMode();

	/* Puts the director at the first checkpoint once the level
	   has been built */
	void StartRunAt(const AZoneCheckpoint& FirstCheckpoint);

	/* Makes a checkpoint the current zone and restart spot */
	void ActivateCheckpoint(const AZoneCheckpoint& Checkpoint);

	/* Sends the director back to the last checkpoint */
	void HandlePlayerCaught();

	/* Wins the game */
	void HandleUploadComplete();

	/* Reloads the level for a fresh run */
	void RestartRun();

protected:
	/* Spawns the level builder if the map does not have one */
	virtual void BeginPlay() override;

private:
	/* Moves the director to the checkpoint and resets the
	   guards and the upload */
	void RestartFromCheckpoint();

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
