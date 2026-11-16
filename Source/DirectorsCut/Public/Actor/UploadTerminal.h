#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UploadTerminal.generated.h"

class ABacklotGameState;
class AGuardCharacter;
class UStaticMeshComponent;
class UTextRenderComponent;

/* The upload terminal at the end of the run. Starting it
   begins the upload of the film; while it runs, guards arrive
   in growing waves from the spawn points. The upload only
   advances while the director stays close, and finishing it
   wins the game. */
UCLASS()
class AUploadTerminal : public AActor
{
	GENERATED_BODY()

public:
	/* Creates the terminal's desk, screen and label */
	AUploadTerminal();

	/* Returns whether E would start the upload from there */
	bool CanBeUsedFrom(const FVector& UserLocation) const;

	/* Begins the upload and the guard waves */
	void StartUpload();

	/* Stops the upload, clears its progress and removes the
	   wave guards */
	void ResetUpload();

protected:
	/* Applies meshes and colors */
	virtual void BeginPlay() override;

	/* Advances the upload and the wave timer */
	virtual void Tick(float DeltaSeconds) override;

private:
	/* Turns the placed spawn points into world locations */
	void BuildGuardSpawnLocations();

	/* Adds progress while the director is close enough */
	void AdvanceUpload(float DeltaSeconds);

	/* Returns whether the director is close enough */
	bool IsPlayerInUploadRange() const;

	/* Counts down to, and spawns, the next wave */
	void UpdateWaves(float DeltaSeconds);

	/* Spawns the next, larger wave of guards */
	void SpawnWave();

	/* Returns how many guards the next wave should add */
	int32 GetNextWaveGuardCount() const;

	/* Spawns one hunting guard at a door */
	void SpawnWaveGuard(const FVector& SpawnLocation);

	/* Returns how many wave guards are still standing */
	int32 CountLivingWaveGuards() const;

	/* Finishes the upload and wins the game */
	void CompleteUpload();

	/* Returns the game state */
	ABacklotGameState* GetBacklotGameState() const;

	/* Desk of the terminal */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> DeskMesh;

	/* Glowing screen of the terminal */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	/* Floating label above the terminal */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UTextRenderComponent> Label;

	/* Whether the upload is running */
	bool bIsUploading;

	/* Whether the upload has finished */
	bool bIsComplete;

	/* Upload progress from 0 to 1 */
	UPROPERTY(VisibleAnywhere, Category = "Custom|State")
	float UploadProgress;

	/* Seconds until the next wave of guards */
	float SecondsUntilNextWave;

	/* Number of waves spawned so far */
	int32 SpawnedWaveCount;

	/* Where wave guards appear, relative to the terminal */
	UPROPERTY(EditAnywhere, Category = "Custom|Waves",
		meta = (MakeEditWidget))
	TArray<FVector> GuardSpawnPoints;

	/* World locations where wave guards appear */
	TArray<FVector> GuardSpawnLocations;

	/* Guards spawned by this terminal's waves */
	TArray<TWeakObjectPtr<AGuardCharacter>> WaveGuards;
};
