#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZoneCheckpoint.generated.h"

class UBoxComponent;

/* A trigger at the start of a zone. Walking through it names
   the zone on the HUD and makes this spot the place the
   director restarts from when caught. */
UCLASS()
class AZoneCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	/* Creates the trigger box */
	AZoneCheckpoint();

	/* Sets the zone this checkpoint starts and its trigger size */
	void Configure(const FString& InZoneName,
		const FVector& TriggerExtent);

	/* Makes the HUD list the controls while in this zone */
	void EnableControlsHelp();

	/* Returns the name of the zone this checkpoint starts */
	const FString& GetZoneName() const;

	/* Returns whether the HUD lists the controls in this zone */
	bool ShowsControlsHelp() const;

private:
	/* Activates this checkpoint when the director enters it */
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	/* Box the director walks through */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/* Name of the zone this checkpoint starts */
	FString ZoneName;

	/* Whether the HUD lists the controls in this zone */
	bool bShowsControlsHelp;
};
