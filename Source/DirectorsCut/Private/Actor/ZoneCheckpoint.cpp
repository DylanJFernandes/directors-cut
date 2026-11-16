#include "Actor/ZoneCheckpoint.h"

#include "Character/DirectorCharacter.h"
#include "Components/BoxComponent.h"
#include "Game/BacklotGameMode.h"

AZoneCheckpoint::AZoneCheckpoint()
	: ZoneName{TEXT("Zone")},
	  bShowsControlsHelp{false},
	  bStartsRun{false}
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(
		TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetCanEverAffectNavigation(false);
	TriggerBox->SetBoxExtent(FVector{300.0f, 1600.0f, 400.0f});
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this,
		&AZoneCheckpoint::HandleTriggerBeginOverlap);
}

void AZoneCheckpoint::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ABacklotGameMode* GameMode{
		GetWorld()->GetAuthGameMode<ABacklotGameMode>()};
	if (GameMode && Cast<ADirectorCharacter>(OtherActor))
	{
		GameMode->ActivateCheckpoint(*this);
	}
}

const FString& AZoneCheckpoint::GetZoneName() const
{
	return ZoneName;
}

bool AZoneCheckpoint::ShowsControlsHelp() const
{
	return bShowsControlsHelp;
}

bool AZoneCheckpoint::StartsRun() const
{
	return bStartsRun;
}
