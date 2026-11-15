#include "Actor/TommyGunPickup.h"

#include "Character/DirectorCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "Game/TommyGunShape.h"
#include "Kismet/GameplayStatics.h"

ATommyGunPickup::ATommyGunPickup()
	: TurnRateDegreesPerSecond{120.0f},
	  BobHeight{15.0f}
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(
		TEXT("Root")));

	GunRoot = CreateDefaultSubobject<USceneComponent>(
		TEXT("GunRoot"));
	GunRoot->SetupAttachment(GetRootComponent());
	GunRoot->SetRelativeScale3D(FVector{2.2f});

	Label = CreateDefaultSubobject<UTextRenderComponent>(
		TEXT("Label"));
	Label->SetupAttachment(GetRootComponent());
	Label->SetRelativeLocation(FVector{0.0f, 0.0f, 90.0f});
	Label->SetRelativeRotation(FRotator{0.0f, 180.0f, 0.0f});
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(40.0f);
	Label->SetText(FText::FromString(TEXT("TOMMY GUN")));
	Label->SetTextRenderColor(FColor::Yellow);
}

void ATommyGunPickup::BeginPlay()
{
	Super::BeginPlay();
	TommyGunShape::Build(*GunRoot);
}

void ATommyGunPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float TimeSeconds{
		static_cast<float>(GetWorld()->GetTimeSeconds())};
	GunRoot->SetRelativeLocationAndRotation(
		FVector{0.0f, 0.0f,
			BobHeight * FMath::Sin(TimeSeconds * 2.5f)},
		FRotator{0.0f,
			TimeSeconds * TurnRateDegreesPerSecond, 0.0f});

	ADirectorCharacter* Director{Cast<ADirectorCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0))};
	const bool bIsDirectorInReach{Director && FVector::Dist(
		Director->GetActorLocation(), GetActorLocation())
		<= UBacklotTuningSettings::Get().TommyGunPickupRadius};
	if (bIsDirectorInReach)
	{
		Director->EquipTommyGun();
		Destroy();
	}
}