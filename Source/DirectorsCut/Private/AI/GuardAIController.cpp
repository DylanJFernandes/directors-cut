#include "AI/GuardAIController.h"

#include "Character/DirectorCharacter.h"
#include "Character/GuardCharacter.h"
#include "Data/BacklotTuningSettings.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

namespace GuardPerceptionConstants
{
	/* Sight is lost a little beyond the range it is gained at,
	   so it does not flicker at the edge */
	constexpr float LoseSightRangeMultiplier{1.15f};
}

AGuardAIController::AGuardAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(
		TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(0.5f);

	UAIPerceptionComponent* GuardPerception{
		CreateDefaultSubobject<UAIPerceptionComponent>(
			TEXT("GuardPerception"))};
	GuardPerception->ConfigureSense(*SightConfig);
	GuardPerception->SetDominantSense(
		SightConfig->GetSenseImplementation());
	GuardPerception->OnTargetPerceptionUpdated.AddDynamic(this,
		&AGuardAIController::HandleTargetPerceptionUpdated);
	SetPerceptionComponent(*GuardPerception);
}

void AGuardAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ApplySightTuning();
}

void AGuardAIController::ApplySightTuning()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	SightConfig->SightRadius = Tuning.GuardSightRange;
	SightConfig->LoseSightRadius = Tuning.GuardSightRange
		* GuardPerceptionConstants::LoseSightRangeMultiplier;
	SightConfig->PeripheralVisionAngleDegrees =
		Tuning.GuardVisionConeHalfAngleDegrees;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
}

void AGuardAIController::HandleTargetPerceptionUpdated(
	AActor* Actor, FAIStimulus Stimulus)
{
	AGuardCharacter* Guard{Cast<AGuardCharacter>(GetPawn())};
	if (!Guard || !Cast<ADirectorCharacter>(Actor))
	{
		return;
	}
	Guard->HandlePlayerSightChanged(
		Stimulus.WasSuccessfullySensed());
}
