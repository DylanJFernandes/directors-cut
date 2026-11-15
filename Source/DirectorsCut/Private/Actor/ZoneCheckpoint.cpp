#include "Actor/ZoneCheckpoint.h"

#include "Character/DirectorCharacter.h"
#include "Components/BoxComponent.h"
#include "Game/BacklotGameMode.h"

AZoneCheckpoint::AZoneCheckpoint()
	: bShowsControlsHelp{false}
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(
		TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetCanEverAffectNavigation(false);
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this,
		&AZoneCheckpoint::HandleTriggerBeginOverlap);
}

void AZoneCheckpoint::Configure(const FString& InZoneName,
	const FVector& TriggerExtent)
{
	ZoneName = InZoneName;
	TriggerBox->SetBoxExtent(TriggerExtent);
}

void AZoneCheckpoint::EnableControlsHelp()
{
	bShowsControlsHelp = true;
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
