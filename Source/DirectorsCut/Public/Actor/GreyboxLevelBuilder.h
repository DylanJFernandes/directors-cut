#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GreyboxLevelBuilder.generated.h"

class AZoneCheckpoint;
class UStaticMeshComponent;

/* Builds the whole greybox backlot when play begins: the New
   York Street, the Western Set, the Soundstage and the Editing
   Bay, with their hook points, guards, checkpoints and the
   upload terminal. Nothing needs to be placed by hand. */
UCLASS()
class AGreyboxLevelBuilder : public AActor
{
	GENERATED_BODY()

public:
	/* Creates the root the level pieces attach to */
	AGreyboxLevelBuilder();

protected:
	/* Builds the level and starts the run */
	virtual void BeginPlay() override;

private:
	// Zones
	/* Adds a sun and sky if the map has no sun */
	void BuildLightingIfMissing();

	/* Adds the walls that close the level in */
	void BuildBoundaryWalls();

	/* Zone 1: tall facades with fire escapes, two guards */
	void BuildNewYorkStreet();

	/* Adds one pair of facing New York buildings */
	void BuildNewYorkBlock(int32 BlockIndex);

	/* Zone 2: open street, balconies, bell tower, four guards */
	void BuildWesternSet();

	/* Adds one pair of facing Western buildings */
	void BuildWesternBlock(int32 BlockIndex);

	/* Adds the bell tower in the middle of the Western street */
	void BuildBellTower();

	/* Zone 3: interior with a catwalk, three guards */
	void BuildSoundstage();

	/* Adds the soundstage catwalks and their hook points */
	void BuildCatwalks();

	/* The finale room: terminal, three doors, cover */
	void BuildEditingBay();

	// Pieces
	/* Adds a solid box given its lowest corner and its size */
	UStaticMeshComponent* AddBox(const FVector& MinimumCorner,
		const FVector& Size, const FLinearColor& Color);

	/* Adds a wall across the level with a doorway in it */
	void AddWallWithDoorway(float WallX, float WallHeight,
		const FLinearColor& Color);

	/* Adds a ledge sticking out of a building front, with a
	   hook point above its outer edge */
	void AddLedgeWithHook(const FVector& LedgeCenterAtFront,
		float OutwardDirection, const FVector2D& LengthAndDepth,
		const FLinearColor& Color);

	/* Adds a film reel hook point */
	void AddHookPoint(const FVector& Location);

	/* Adds a guard that patrols the given ground points */
	void AddGuard(const TArray<FVector2D>& PatrolPoints);

	/* Adds a checkpoint that starts a zone */
	AZoneCheckpoint* AddCheckpoint(const FString& ZoneName,
		const FVector2D& GroundPoint,
		const FVector& TriggerExtent);

	/* Adds a text sign facing players coming from the start */
	void AddSign(const FString& Text, const FVector& Location,
		float TextSize);

	/* Checkpoint the run starts at */
	TWeakObjectPtr<AZoneCheckpoint> FirstCheckpoint;
};
