#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GuardAIController.generated.h"

class UAISenseConfig_Sight;

/* AI controller for a studio guard. Owns the AI Perception
   sight sense and tells its guard when it gains or loses sight
   of the director. */
UCLASS()
class AGuardAIController : public AAIController
{
	GENERATED_BODY()

public:
	/* Creates the perception component and its sight sense */
	AGuardAIController();

protected:
	/* Applies the tuned sight range and vision cone */
	virtual void OnPossess(APawn* InPawn) override;

private:
	/* Copies the sight tuning values into the sight sense */
	void ApplySightTuning();

	/* Forwards sight of the director to the guard */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor,
		FAIStimulus Stimulus);

	/* Sight sense settings used by the perception component */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
};
