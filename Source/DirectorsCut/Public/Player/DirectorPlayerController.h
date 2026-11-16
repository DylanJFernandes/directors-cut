#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DirectorPlayerController.generated.h"

class ABacklotHUD;

/* Player controller for the director. Holds the HUD reference
   and the keys that are not character actions: restart and
   the debug vision cone toggle. */
UCLASS()
class ADirectorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/* Returns this controller's HUD, caching it on first use */
	ABacklotHUD* GetBacklotHUD();

protected:
	/* Captures the mouse for gameplay */
	virtual void BeginPlay() override;

	/* Binds the restart and debug keys */
	virtual void SetupInputComponent() override;

private:
	/* Restarts the run once the game is won or lost */
	void HandleRestartPressed();

	/* Switches the guard vision cones on or off */
	void HandleToggleVisionConesPressed();

	/* HUD spawned for this controller */
	TWeakObjectPtr<ABacklotHUD> CachedBacklotHUD;
};
