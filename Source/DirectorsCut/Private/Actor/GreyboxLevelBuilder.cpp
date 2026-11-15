#include "Actor/GreyboxLevelBuilder.h"

#include "Actor/GrappleHookPoint.h"
#include "Actor/TommyGunPickup.h"
#include "Actor/UploadTerminal.h"
#include "Actor/ZoneCheckpoint.h"
#include "Character/GuardCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Game/BacklotGameMode.h"
#include "Game/GreyboxShapeLibrary.h"

namespace BacklotLayout
{
	/* Half of the level's width; the side walls stand here */
	constexpr float LevelHalfWidth{1800.0f};

	/* Where each zone starts along the level's X axis */
	constexpr float NewYorkStartX{-400.0f};
	constexpr float WesternStartX{5300.0f};
	constexpr float SoundstageStartX{11400.0f};
	constexpr float EditingBayStartX{16400.0f};
	constexpr float LevelEndX{19200.0f};

	/* Thickness of the ground slabs */
	constexpr float GroundThickness{100.0f};

	/* Height of the walls that close the level in */
	constexpr float BoundaryWallHeight{2600.0f};

	/* Height above the ground at which characters are spawned */
	constexpr float CharacterSpawnHeight{96.0f};

	/* Half width and height of the doorways between zones */
	constexpr float DoorwayHalfWidth{200.0f};
	constexpr float DoorwayHeight{400.0f};

	/* Where a hook point sits relative to the ledge it serves */
	constexpr float HookHeightAboveLedge{190.0f};
	constexpr float HookDistanceInsideEdge{110.0f};

	/* Where a rooftop hook point sits relative to the roof edge */
	constexpr float RoofHookHeight{230.0f};
	constexpr float RoofHookDistanceInsideEdge{50.0f};

	/* Thickness of ledges, platforms and catwalks */
	constexpr float PlatformThickness{24.0f};

	const FLinearColor Asphalt{0.11f, 0.11f, 0.13f};
	const FLinearColor BrickRed{0.32f, 0.12f, 0.09f};
	const FLinearColor BrickGrey{0.25f, 0.26f, 0.3f};
	const FLinearColor Metal{0.03f, 0.03f, 0.04f};
	const FLinearColor Hazard{0.9f, 0.7f, 0.05f};
	const FLinearColor Sand{0.5f, 0.38f, 0.2f};
	const FLinearColor Wood{0.33f, 0.2f, 0.1f};
	const FLinearColor PaleWood{0.5f, 0.36f, 0.2f};
	const FLinearColor StageFloor{0.1f, 0.1f, 0.11f};
	const FLinearColor StageWall{0.2f, 0.22f, 0.26f};
	const FLinearColor Catwalk{0.55f, 0.45f, 0.1f};
	const FLinearColor Crate{0.4f, 0.3f, 0.18f};
	const FLinearColor BayFloor{0.07f, 0.1f, 0.18f};
	const FLinearColor BayWall{0.12f, 0.18f, 0.3f};
	const FLinearColor GuardDoor{0.8f, 0.08f, 0.08f};
	const FLinearColor Boundary{0.12f, 0.12f, 0.14f};
}

AGreyboxLevelBuilder::AGreyboxLevelBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* LevelRoot{
		CreateDefaultSubobject<USceneComponent>(TEXT("LevelRoot"))};
	LevelRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(LevelRoot);
}

void AGreyboxLevelBuilder::BeginPlay()
{
	Super::BeginPlay();
	BuildLightingIfMissing();
	BuildBoundaryWalls();
	BuildNewYorkStreet();
	BuildWesternSet();
	BuildSoundstage();
	BuildEditingBay();

	ABacklotGameMode* GameMode{
		GetWorld()->GetAuthGameMode<ABacklotGameMode>()};
	if (GameMode && FirstCheckpoint.IsValid())
	{
		GameMode->StartRunAt(*FirstCheckpoint);
	}
}

void AGreyboxLevelBuilder::BuildLightingIfMissing()
{
	const TActorIterator<ADirectionalLight> SunIterator{GetWorld()};
	if (SunIterator)
	{
		return;
	}
	ADirectionalLight* Sun{
		GetWorld()->SpawnActor<ADirectionalLight>(
			FVector::ZeroVector, FRotator{-48.0f, 32.0f, 0.0f})};
	Sun->GetLightComponent()->SetMobility(
		EComponentMobility::Movable);
	Sun->GetLightComponent()->SetIntensity(6.0f);

	ASkyLight* SkyLight{GetWorld()->SpawnActor<ASkyLight>()};
	SkyLight->GetLightComponent()->SetMobility(
		EComponentMobility::Movable);
	SkyLight->GetLightComponent()->bRealTimeCapture = true;
	SkyLight->GetLightComponent()->MarkRenderStateDirty();

	GetWorld()->SpawnActor<ASkyAtmosphere>();
	GetWorld()->SpawnActor<AExponentialHeightFog>();
}

void AGreyboxLevelBuilder::BuildBoundaryWalls()
{
	using namespace BacklotLayout;
	const float WallThickness{100.0f};
	const float LevelLength{LevelEndX - NewYorkStartX};
	const FVector SideWallSize{
		LevelLength, WallThickness, BoundaryWallHeight};
	AddBox(FVector{NewYorkStartX, -LevelHalfWidth - WallThickness,
		0.0f}, SideWallSize, Boundary);
	AddBox(FVector{NewYorkStartX, LevelHalfWidth, 0.0f},
		SideWallSize, Boundary);

	const FVector EndWallSize{WallThickness,
		LevelHalfWidth * 2.0f, BoundaryWallHeight};
	AddBox(FVector{NewYorkStartX - WallThickness, -LevelHalfWidth,
		0.0f}, EndWallSize, Boundary);
	AddBox(FVector{LevelEndX, -LevelHalfWidth, 0.0f}, EndWallSize,
		Boundary);
}

void AGreyboxLevelBuilder::BuildNewYorkStreet()
{
	using namespace BacklotLayout;
	AddBox(FVector{NewYorkStartX, -LevelHalfWidth,
		-GroundThickness}, FVector{WesternStartX - NewYorkStartX,
		LevelHalfWidth * 2.0f, GroundThickness}, Asphalt);
	const int32 BlockCount{5};
	for (int32 BlockIndex{0}; BlockIndex < BlockCount; ++BlockIndex)
	{
		BuildNewYorkBlock(BlockIndex);
	}

	AddBox(FVector{1000.0f, -600.0f, 0.0f},
		FVector{100.0f, 1200.0f, 450.0f}, Hazard);
	AddSign(TEXT("ROAD CLOSED - GRAPPLE UP  [E]"),
		FVector{990.0f, 0.0f, 520.0f}, 70.0f);
	AddSign(TEXT("Q: FLASH THEM   F: CUT!"),
		FVector{1700.0f, 0.0f, 620.0f}, 70.0f);
	AddBox(FVector{2300.0f, -80.0f, 0.0f},
		FVector{160.0f, 160.0f, 130.0f}, Crate);
	AddBox(FVector{3000.0f, -560.0f, 0.0f},
		FVector{300.0f, 150.0f, 180.0f}, Crate);
	AddBox(FVector{3900.0f, 200.0f, 0.0f},
		FVector{200.0f, 160.0f, 130.0f}, Crate);
	AddBox(FVector{5000.0f, -600.0f, 700.0f},
		FVector{100.0f, 1200.0f, 300.0f}, BrickGrey);
	AddSign(TEXT("WESTERN SET"),
		FVector{4990.0f, 0.0f, 850.0f}, 120.0f);

	AddGuard({FVector2D{1900.0f, -300.0f},
		FVector2D{2900.0f, -300.0f}, FVector2D{2900.0f, 300.0f},
		FVector2D{1900.0f, 300.0f}});
	AddGuard({FVector2D{3400.0f, 0.0f},
		FVector2D{4600.0f, 0.0f}});

	FirstCheckpoint = AddCheckpoint(TEXT("New York Street"),
		FVector2D{200.0f, 0.0f},
		FVector{150.0f, 600.0f, 300.0f});
	FirstCheckpoint->EnableControlsHelp();
}

void AGreyboxLevelBuilder::BuildNewYorkBlock(int32 BlockIndex)
{
	using namespace BacklotLayout;
	const TArray<float> LeftHeights{
		1400.0f, 1000.0f, 1600.0f, 1200.0f, 1400.0f};
	const TArray<float> RightHeights{
		1200.0f, 1600.0f, 1000.0f, 1400.0f, 1200.0f};
	const float BlockLength{1000.0f};
	const float StreetHalfWidth{600.0f};
	const float BlockStartX{BlockIndex * BlockLength};
	const float BlockCenterX{BlockStartX + BlockLength * 0.5f};
	const FVector2D FootprintSize{BlockLength - 20.0f,
		LevelHalfWidth - StreetHalfWidth};
	const bool bIsEvenBlock{BlockIndex % 2 == 0};

	const float LeftHeight{LeftHeights[BlockIndex]};
	AddBox(FVector{BlockStartX, -LevelHalfWidth, 0.0f},
		FVector{FootprintSize.X, FootprintSize.Y, LeftHeight},
		bIsEvenBlock ? BrickRed : BrickGrey);
	AddLedgeWithHook(
		FVector{BlockCenterX, -StreetHalfWidth, 520.0f}, 1.0f,
		FVector2D{700.0f, 250.0f}, Metal);
	AddHookPoint(FVector{BlockCenterX,
		-StreetHalfWidth - RoofHookDistanceInsideEdge,
		LeftHeight + RoofHookHeight});

	const float RightHeight{RightHeights[BlockIndex]};
	AddBox(FVector{BlockStartX, StreetHalfWidth, 0.0f},
		FVector{FootprintSize.X, FootprintSize.Y, RightHeight},
		bIsEvenBlock ? BrickGrey : BrickRed);
	AddLedgeWithHook(
		FVector{BlockCenterX, StreetHalfWidth, 520.0f}, -1.0f,
		FVector2D{700.0f, 250.0f}, Metal);
	AddHookPoint(FVector{BlockCenterX,
		StreetHalfWidth + RoofHookDistanceInsideEdge,
		RightHeight + RoofHookHeight});
}

void AGreyboxLevelBuilder::BuildWesternSet()
{
	using namespace BacklotLayout;
	AddBox(FVector{WesternStartX, -LevelHalfWidth,
		-GroundThickness}, FVector{SoundstageStartX - WesternStartX,
		LevelHalfWidth * 2.0f, GroundThickness}, Sand);
	const int32 BlockCount{4};
	for (int32 BlockIndex{0}; BlockIndex < BlockCount; ++BlockIndex)
	{
		BuildWesternBlock(BlockIndex);
	}
	BuildBellTower();

	AddBox(FVector{6600.0f, -200.0f, 0.0f},
		FVector{300.0f, 100.0f, 90.0f}, PaleWood);
	AddBox(FVector{7400.0f, 300.0f, 0.0f},
		FVector{350.0f, 180.0f, 150.0f}, PaleWood);
	AddBox(FVector{9600.0f, -100.0f, 0.0f},
		FVector{200.0f, 200.0f, 140.0f}, Crate);
	AddBox(FVector{10400.0f, 350.0f, 0.0f},
		FVector{300.0f, 120.0f, 120.0f}, PaleWood);

	AddGuard({FVector2D{6000.0f, -500.0f},
		FVector2D{7600.0f, -500.0f}, FVector2D{7600.0f, 500.0f},
		FVector2D{6000.0f, 500.0f}});
	AddGuard({FVector2D{7900.0f, -600.0f},
		FVector2D{8900.0f, -600.0f}, FVector2D{8900.0f, 600.0f},
		FVector2D{7900.0f, 600.0f}});
	AddGuard({FVector2D{10800.0f, 500.0f},
		FVector2D{9400.0f, 500.0f}, FVector2D{9400.0f, -500.0f},
		FVector2D{10800.0f, -500.0f}});
	AddGuard({FVector2D{11050.0f, -700.0f},
		FVector2D{11050.0f, 700.0f}});

	AddSign(TEXT("STAY HIGH - DROP ON THEM WITH F"),
		FVector{5650.0f, 0.0f, 520.0f}, 70.0f);
	AddCheckpoint(TEXT("Western Set"),
		FVector2D{WesternStartX + 150.0f, 0.0f},
		FVector{100.0f, LevelHalfWidth, 400.0f});
}

void AGreyboxLevelBuilder::BuildWesternBlock(int32 BlockIndex)
{
	using namespace BacklotLayout;
	const TArray<float> LeftHeights{
		600.0f, 700.0f, 600.0f, 750.0f};
	const TArray<float> RightHeights{
		700.0f, 600.0f, 750.0f, 600.0f};
	const float BlockLength{1100.0f};
	const float BlockSpacing{1400.0f};
	const float StreetHalfWidth{1100.0f};
	const float BalconyHeight{330.0f};
	const float BlockStartX{5700.0f + BlockIndex * BlockSpacing};
	const float BlockCenterX{BlockStartX + BlockLength * 0.5f};
	const float BuildingDepth{LevelHalfWidth - StreetHalfWidth};

	const float LeftHeight{LeftHeights[BlockIndex]};
	AddBox(FVector{BlockStartX, -LevelHalfWidth, 0.0f},
		FVector{BlockLength, BuildingDepth, LeftHeight}, Wood);
	AddLedgeWithHook(
		FVector{BlockCenterX, -StreetHalfWidth, BalconyHeight},
		1.0f, FVector2D{1000.0f, 260.0f}, PaleWood);
	AddHookPoint(FVector{BlockCenterX,
		-StreetHalfWidth - RoofHookDistanceInsideEdge,
		LeftHeight + RoofHookHeight});

	const float RightHeight{RightHeights[BlockIndex]};
	AddBox(FVector{BlockStartX, StreetHalfWidth, 0.0f},
		FVector{BlockLength, BuildingDepth, RightHeight}, Wood);
	AddLedgeWithHook(
		FVector{BlockCenterX, StreetHalfWidth, BalconyHeight},
		-1.0f, FVector2D{1000.0f, 260.0f}, PaleWood);
	AddHookPoint(FVector{BlockCenterX,
		StreetHalfWidth + RoofHookDistanceInsideEdge,
		RightHeight + RoofHookHeight});
}

void AGreyboxLevelBuilder::BuildBellTower()
{
	using namespace BacklotLayout;
	const FVector2D TowerCenter{8400.0f, 0.0f};
	const float TowerHeight{1500.0f};
	const float PlatformHalfSize{300.0f};
	AddBox(FVector{TowerCenter.X - 125.0f, -125.0f, 0.0f},
		FVector{250.0f, 250.0f, TowerHeight}, Wood);
	AddBox(FVector{TowerCenter.X - PlatformHalfSize,
		-PlatformHalfSize, TowerHeight},
		FVector{PlatformHalfSize * 2.0f, PlatformHalfSize * 2.0f,
			PlatformThickness}, PaleWood);
	AddBox(FVector{TowerCenter.X - 60.0f, -60.0f,
		TowerHeight + PlatformThickness},
		FVector{120.0f, 120.0f, 140.0f}, Hazard);

	const float HookOffset{
		PlatformHalfSize - HookDistanceInsideEdge};
	const float HookHeight{TowerHeight + PlatformThickness
		+ HookHeightAboveLedge};
	AddHookPoint(FVector{TowerCenter.X - HookOffset, 0.0f,
		HookHeight});
	AddHookPoint(FVector{TowerCenter.X + HookOffset, 0.0f,
		HookHeight});
	AddSign(TEXT("BELL TOWER"),
		FVector{TowerCenter.X - 130.0f, 0.0f, 900.0f}, 60.0f);
}

void AGreyboxLevelBuilder::BuildSoundstage()
{
	using namespace BacklotLayout;
	const float StageLength{EditingBayStartX - SoundstageStartX};
	const float WallHeight{1500.0f};
	const float InnerHalfWidth{1500.0f};
	AddBox(FVector{SoundstageStartX, -LevelHalfWidth,
		-GroundThickness}, FVector{StageLength,
		LevelHalfWidth * 2.0f, GroundThickness}, StageFloor);
	AddWallWithDoorway(SoundstageStartX, WallHeight, StageWall);
	AddSign(TEXT("SOUNDSTAGE 7"),
		FVector{SoundstageStartX - 10.0f, 0.0f, 600.0f}, 120.0f);
	AddBox(FVector{SoundstageStartX, -InnerHalfWidth - 100.0f,
		0.0f}, FVector{StageLength, 100.0f, WallHeight}, StageWall);
	AddBox(FVector{SoundstageStartX, InnerHalfWidth, 0.0f},
		FVector{StageLength, 100.0f, WallHeight}, StageWall);

	const float BeamSpacing{700.0f};
	for (float BeamX{SoundstageStartX + 400.0f};
		BeamX < EditingBayStartX; BeamX += BeamSpacing)
	{
		AddBox(FVector{BeamX, -InnerHalfWidth, WallHeight},
			FVector{100.0f, InnerHalfWidth * 2.0f, 80.0f}, Metal);
	}
	BuildCatwalks();

	AddBox(FVector{12800.0f, -900.0f, 0.0f},
		FVector{600.0f, 40.0f, 400.0f}, PaleWood);
	AddBox(FVector{12600.0f, 500.0f, 0.0f},
		FVector{250.0f, 250.0f, 220.0f}, Crate);
	AddBox(FVector{14500.0f, -600.0f, 0.0f},
		FVector{300.0f, 300.0f, 250.0f}, Crate);
	AddBox(FVector{15000.0f, 700.0f, 0.0f},
		FVector{250.0f, 400.0f, 200.0f}, Crate);
	AddBox(FVector{13400.0f, 1000.0f, 0.0f},
		FVector{500.0f, 40.0f, 350.0f}, PaleWood);

	AddGuard({FVector2D{12300.0f, -1100.0f},
		FVector2D{15600.0f, -1100.0f}});
	AddGuard({FVector2D{15600.0f, 1200.0f},
		FVector2D{12300.0f, 1200.0f}});
	AddGuard({FVector2D{13000.0f, 300.0f},
		FVector2D{14900.0f, 300.0f}, FVector2D{14900.0f, -300.0f},
		FVector2D{13000.0f, -300.0f}});

	AddSign(TEXT("QUIET ON SET - USE THE CATWALK"),
		FVector{12000.0f, 0.0f, 1050.0f}, 70.0f);
	AddCheckpoint(TEXT("Soundstage"),
		FVector2D{SoundstageStartX + 250.0f, 0.0f},
		FVector{80.0f, 400.0f, 300.0f});
}

void AGreyboxLevelBuilder::BuildCatwalks()
{
	using namespace BacklotLayout;
	const float CatwalkHeight{700.0f};
	const float CatwalkHalfWidth{110.0f};
	const FVector2D MainSpanX{12100.0f, 15700.0f};
	const float CrossX{13900.0f};
	const float CrossHalfLength{1300.0f};
	AddBox(FVector{MainSpanX.X, -CatwalkHalfWidth, CatwalkHeight},
		FVector{MainSpanX.Y - MainSpanX.X,
			CatwalkHalfWidth * 2.0f, PlatformThickness}, Catwalk);
	AddBox(FVector{CrossX - CatwalkHalfWidth, -CrossHalfLength,
		CatwalkHeight}, FVector{CatwalkHalfWidth * 2.0f,
		CrossHalfLength * 2.0f, PlatformThickness}, Catwalk);

	const float PostSize{40.0f};
	for (const double PostX : {MainSpanX.X, MainSpanX.Y - PostSize})
	{
		AddBox(FVector{PostX, -PostSize * 0.5f, 0.0f},
			FVector{PostSize, PostSize, CatwalkHeight}, Metal);
	}

	const float HookHeight{CatwalkHeight + PlatformThickness
		+ HookHeightAboveLedge};
	AddHookPoint(FVector{
		MainSpanX.X + HookDistanceInsideEdge, 0.0f, HookHeight});
	AddHookPoint(FVector{
		MainSpanX.Y - HookDistanceInsideEdge, 0.0f, HookHeight});
	AddHookPoint(FVector{CrossX,
		-CrossHalfLength + HookDistanceInsideEdge, HookHeight});
	AddHookPoint(FVector{CrossX,
		CrossHalfLength - HookDistanceInsideEdge, HookHeight});
	AddHookPoint(FVector{13000.0f, 0.0f, HookHeight});
	AddHookPoint(FVector{14800.0f, 0.0f, HookHeight});
}

void AGreyboxLevelBuilder::BuildEditingBay()
{
	using namespace BacklotLayout;
	const float BayLength{LevelEndX - EditingBayStartX};
	const float WallHeight{900.0f};
	const float InnerHalfWidth{1300.0f};
	const FVector2D TerminalPoint{17900.0f, 0.0f};
	AddBox(FVector{EditingBayStartX, -LevelHalfWidth,
		-GroundThickness}, FVector{BayLength,
		LevelHalfWidth * 2.0f, GroundThickness}, BayFloor);
	AddWallWithDoorway(EditingBayStartX, 1500.0f, StageWall);
	AddSign(TEXT("EDITING BAY"),
		FVector{EditingBayStartX - 10.0f, 0.0f, 600.0f}, 120.0f);
	AddBox(FVector{EditingBayStartX, -InnerHalfWidth - 100.0f,
		0.0f}, FVector{BayLength, 100.0f, WallHeight}, BayWall);
	AddBox(FVector{EditingBayStartX, InnerHalfWidth, 0.0f},
		FVector{BayLength, 100.0f, WallHeight}, BayWall);

	AddBox(FVector{17750.0f, -InnerHalfWidth - 10.0f, 0.0f},
		FVector{300.0f, 20.0f, 350.0f}, GuardDoor);
	AddBox(FVector{17750.0f, InnerHalfWidth - 10.0f, 0.0f},
		FVector{300.0f, 20.0f, 350.0f}, GuardDoor);
	AddBox(FVector{LevelEndX - 10.0f, -150.0f, 0.0f},
		FVector{20.0f, 300.0f, 350.0f}, GuardDoor);

	AddBox(FVector{17500.0f, -500.0f, 0.0f},
		FVector{40.0f, 380.0f, 130.0f}, Crate);
	AddBox(FVector{17500.0f, 120.0f, 0.0f},
		FVector{40.0f, 380.0f, 130.0f}, Crate);
	AddBox(FVector{18150.0f, -620.0f, 0.0f},
		FVector{400.0f, 40.0f, 130.0f}, Crate);
	AddBox(FVector{18150.0f, 580.0f, 0.0f},
		FVector{400.0f, 40.0f, 130.0f}, Crate);

	for (const float PerchSide : {-1.0f, 1.0f})
	{
		const float PerchY{PerchSide * 900.0f};
		AddBox(FVector{16970.0f, PerchY - 30.0f, 0.0f},
			FVector{60.0f, 60.0f, 500.0f}, Metal);
		AddBox(FVector{16850.0f, PerchY - 150.0f, 500.0f},
			FVector{300.0f, 300.0f, PlatformThickness}, Catwalk);
		AddHookPoint(FVector{17000.0f, PerchY,
			500.0f + PlatformThickness + HookHeightAboveLedge});
	}

	AUploadTerminal* Terminal{
		GetWorld()->SpawnActor<AUploadTerminal>(
			GetActorTransform().TransformPosition(
				FVector{TerminalPoint.X, TerminalPoint.Y, 50.0f}),
			GetActorRotation())};
	Terminal->SetGuardSpawnLocations({
		GetActorTransform().TransformPosition(FVector{17900.0f,
			-InnerHalfWidth + 150.0f, CharacterSpawnHeight}),
		GetActorTransform().TransformPosition(FVector{17900.0f,
			InnerHalfWidth - 150.0f, CharacterSpawnHeight}),
		GetActorTransform().TransformPosition(FVector{
			LevelEndX - 150.0f, 0.0f, CharacterSpawnHeight})});

	GetWorld()->SpawnActor<ATommyGunPickup>(
		GetActorTransform().TransformPosition(
			FVector{17250.0f, 0.0f, 130.0f}), GetActorRotation());
	AddSign(TEXT("START THE UPLOAD AND STAY CLOSE"),
		FVector{17600.0f, 0.0f, 420.0f}, 60.0f);
	AddCheckpoint(TEXT("Editing Bay"),
		FVector2D{EditingBayStartX + 250.0f, 0.0f},
		FVector{80.0f, 400.0f, 300.0f});
}

UStaticMeshComponent* AGreyboxLevelBuilder::AddBox(
	const FVector& MinimumCorner, const FVector& Size,
	const FLinearColor& Color)
{
	UStaticMeshComponent* Box{
		NewObject<UStaticMeshComponent>(this)};
	Box->SetMobility(EComponentMobility::Static);
	Box->SetStaticMesh(GreyboxShapeLibrary::GetCubeMesh());
	Box->SetupAttachment(GetRootComponent());
	Box->SetRelativeLocation(MinimumCorner + Size * 0.5f);
	Box->SetRelativeScale3D(
		Size / GreyboxShapeLibrary::BasicShapeSize);
	GreyboxShapeLibrary::ApplyFlatColor(Box, Color);
	Box->RegisterComponent();
	return Box;
}

void AGreyboxLevelBuilder::AddWallWithDoorway(float WallX,
	float WallHeight, const FLinearColor& Color)
{
	using namespace BacklotLayout;
	const float WallThickness{100.0f};
	const float SideLength{LevelHalfWidth - DoorwayHalfWidth};
	AddBox(FVector{WallX, -LevelHalfWidth, 0.0f},
		FVector{WallThickness, SideLength, WallHeight}, Color);
	AddBox(FVector{WallX, DoorwayHalfWidth, 0.0f},
		FVector{WallThickness, SideLength, WallHeight}, Color);
	AddBox(FVector{WallX, -DoorwayHalfWidth, DoorwayHeight},
		FVector{WallThickness, DoorwayHalfWidth * 2.0f,
			WallHeight - DoorwayHeight}, Color);
}

void AGreyboxLevelBuilder::AddLedgeWithHook(
	const FVector& LedgeCenterAtFront, float OutwardDirection,
	const FVector2D& LengthAndDepth, const FLinearColor& Color)
{
	using namespace BacklotLayout;
	const float Length{static_cast<float>(LengthAndDepth.X)};
	const float Depth{static_cast<float>(LengthAndDepth.Y)};
	const float OuterEdgeY{static_cast<float>(
		LedgeCenterAtFront.Y + OutwardDirection * Depth)};
	const float MinimumY{FMath::Min(
		static_cast<float>(LedgeCenterAtFront.Y), OuterEdgeY)};
	AddBox(FVector{LedgeCenterAtFront.X - Length * 0.5f, MinimumY,
		LedgeCenterAtFront.Z - PlatformThickness},
		FVector{Length, Depth, PlatformThickness}, Color);
	AddHookPoint(FVector{LedgeCenterAtFront.X,
		(LedgeCenterAtFront.Y + OuterEdgeY) * 0.5f,
		LedgeCenterAtFront.Z + HookHeightAboveLedge});
}

void AGreyboxLevelBuilder::AddHookPoint(const FVector& Location)
{
	GetWorld()->SpawnActor<AGrappleHookPoint>(
		GetActorTransform().TransformPosition(Location),
		FRotator::ZeroRotator);
}

void AGreyboxLevelBuilder::AddGuard(
	const TArray<FVector2D>& PatrolPoints)
{
	TArray<FVector> PatrolLocations;
	for (const FVector2D& PatrolPoint : PatrolPoints)
	{
		PatrolLocations.Add(GetActorTransform().TransformPosition(
			FVector{PatrolPoint.X, PatrolPoint.Y,
				BacklotLayout::CharacterSpawnHeight}));
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod
			::AdjustIfPossibleButAlwaysSpawn;
	AGuardCharacter* Guard{
		GetWorld()->SpawnActor<AGuardCharacter>(PatrolLocations[0],
			GetActorRotation(), SpawnParameters)};
	if (Guard)
	{
		Guard->SetPatrolLocations(PatrolLocations);
	}
}

AZoneCheckpoint* AGreyboxLevelBuilder::AddCheckpoint(
	const FString& ZoneName, const FVector2D& GroundPoint,
	const FVector& TriggerExtent)
{
	AZoneCheckpoint* Checkpoint{
		GetWorld()->SpawnActor<AZoneCheckpoint>(
			GetActorTransform().TransformPosition(FVector{
				GroundPoint.X, GroundPoint.Y,
				BacklotLayout::CharacterSpawnHeight}),
			GetActorRotation())};
	Checkpoint->Configure(ZoneName, TriggerExtent);
	return Checkpoint;
}

void AGreyboxLevelBuilder::AddSign(const FString& Text,
	const FVector& Location, float TextSize)
{
	UTextRenderComponent* Sign{
		NewObject<UTextRenderComponent>(this)};
	Sign->SetupAttachment(GetRootComponent());
	Sign->SetRelativeLocation(Location);
	Sign->SetRelativeRotation(FRotator{0.0f, 180.0f, 0.0f});
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetWorldSize(TextSize);
	Sign->SetText(FText::FromString(Text));
	Sign->SetTextRenderColor(FColor::White);
	Sign->RegisterComponent();
}
