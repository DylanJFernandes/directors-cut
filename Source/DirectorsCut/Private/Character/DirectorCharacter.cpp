#include "Character/DirectorCharacter.h"

#include "Character/GuardCharacter.h"

#include "Actor/CameraRigTurret.h"
#include "Actor/UploadTerminal.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Component/BeamMeshComponent.h"
#include "Component/CameraFlashComponent.h"
#include "Component/DirectorHealthComponent.h"
#include "Component/GrappleComponent.h"
#include "Component/MegaphoneComponent.h"
#include "Component/TakedownComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/BacklotGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Player/DirectorPlayerController.h"
#include "TimerManager.h"
#include "UI/HUD/BacklotHUD.h"
#include "UObject/ConstructorHelpers.h"

namespace DirectorConstants
{
	/* Capsule half height while standing and while crouched */
	constexpr float StandingHalfHeight{90.0f};
	constexpr float CrouchedHalfHeight{55.0f};

	/* Vertical mesh scale while crouched */
	constexpr float CrouchedMeshHeightScale{0.62f};

	/* Seconds the bright flash light stays on */
	constexpr float FlashLightSeconds{0.15f};

	/* Brightness of the flash light */
	constexpr float FlashLightIntensity{400000.0f};

	/* Thickness of the film reel tether */
	constexpr float TetherThickness{5.0f};

	/* Push the director gets from a guard's shot */
	constexpr float ShotKnockbackSpeed{250.0f};
}

ADirectorCharacter::ADirectorCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->InitCapsuleSize(35.0f,
		DirectorConstants::StandingHalfHeight);

	UCharacterMovementComponent* Movement{
		GetCharacterMovement()};
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator{0.0f, 540.0f, 0.0f};
	Movement->NavAgentProps.bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(
		DirectorConstants::CrouchedHalfHeight);
	Movement->bCanWalkOffLedgesWhenCrouching = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		DirectorMeshFinder{TEXT(
			"/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"
			".SKM_Quinn_Simple")};
	static ConstructorHelpers::FClassFinder<UAnimInstance>
		AnimationFinder{TEXT(
			"/Game/Characters/Mannequins/Anims/Unarmed"
			"/ABP_Unarmed")};
	GetMesh()->SetSkeletalMesh(DirectorMeshFinder.Object);
	GetMesh()->SetAnimInstanceClass(AnimationFinder.Class);
	GetMesh()->SetRelativeLocationAndRotation(
		FVector{0.0f, 0.0f,
			-DirectorConstants::StandingHalfHeight},
		FRotator{0.0f, -90.0f, 0.0f});

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
		TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector{0.0f, 0.0f, 60.0f});
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(
		TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom,
		USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(
		TEXT("FlashLight"));
	FlashLight->SetupAttachment(GetCapsuleComponent());
	FlashLight->SetRelativeLocation(FVector{60.0f, 0.0f, 50.0f});
	FlashLight->SetIntensity(0.0f);
	FlashLight->SetAttenuationRadius(1600.0f);
	FlashLight->SetCastShadows(false);

	GrappleTether = CreateDefaultSubobject<UBeamMeshComponent>(
		TEXT("GrappleTether"));
	GrappleTether->SetupAttachment(GetCapsuleComponent());

	Health = CreateDefaultSubobject<UDirectorHealthComponent>(
		TEXT("Health"));
	Grapple = CreateDefaultSubobject<UGrappleComponent>(
		TEXT("Grapple"));
	CameraFlash = CreateDefaultSubobject<UCameraFlashComponent>(
		TEXT("CameraFlash"));
	Takedown = CreateDefaultSubobject<UTakedownComponent>(
		TEXT("Takedown"));
	Megaphone = CreateDefaultSubobject<UMegaphoneComponent>(
		TEXT("Megaphone"));
}

void ADirectorCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyTuning();
	GrappleTether->SetBeamThickness(
		DirectorConstants::TetherThickness);
	GrappleTether->SetBeamColor(FLinearColor{0.9f, 0.8f, 0.5f});
	Health->OnHealthDepleted.BindUObject(this,
		&ADirectorCharacter::HandleHealthDepleted);
}

void ADirectorCharacter::ApplyTuning()
{
	const UBacklotTuningSettings& Tuning{
		UBacklotTuningSettings::Get()};
	UCharacterMovementComponent* Movement{
		GetCharacterMovement()};
	Movement->MaxWalkSpeed = Tuning.PlayerRunSpeed;
	Movement->MaxWalkSpeedCrouched = Tuning.PlayerCrouchSpeed;
	Movement->JumpZVelocity = Tuning.PlayerJumpVelocity;
	Movement->AirControl = Tuning.PlayerAirControl;
	CameraBoom->TargetArmLength = Tuning.CameraBoomLength;
}

void ADirectorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateGrappleTether();
	DismountIfKnockedOffCameraRig();
}

void ADirectorCharacter::DismountIfKnockedOffCameraRig()
{
	const bool bWasKnockedOff{MountedCameraRig.IsValid()
		&& GetCharacterMovement()->MovementMode != MOVE_None};
	if (bWasKnockedOff)
	{
		DismountCameraRig();
	}
}

void ADirectorCharacter::UpdateGrappleTether()
{
	if (!Grapple->IsGrappling())
	{
		GrappleTether->HideBeam();
		return;
	}
	GrappleTether->ShowBeamBetween(GetActorLocation(),
		Grapple->GetGrappleDestination());
}

void ADirectorCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInputActions();
	AddMappingContext();
	BindInputActions(PlayerInputComponent);
}

void ADirectorCharacter::CreateInputActions()
{
	DirectorMappingContext = NewObject<UInputMappingContext>(this);

	MoveAction = NewObject<UInputAction>(this);
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);

	LookAction = NewObject<UInputAction>(this);
	LookAction->ValueType = EInputActionValueType::Axis2D;
	DirectorMappingContext->MapKey(LookAction, EKeys::Mouse2D);

	JumpAction = CreateButtonAction(EKeys::SpaceBar);
	CrouchAction = CreateButtonAction(EKeys::C);
	DirectorMappingContext->MapKey(CrouchAction,
		EKeys::LeftControl);
	InteractAction = CreateButtonAction(EKeys::E);
	FlashAction = CreateButtonAction(EKeys::Q);
	TakedownAction = CreateButtonAction(EKeys::F);
	MegaphoneAction = CreateButtonAction(EKeys::G);
	FireAction = CreateButtonAction(EKeys::LeftMouseButton);
}

UInputAction* ADirectorCharacter::CreateButtonAction(
	const FKey& Key)
{
	UInputAction* ButtonAction{NewObject<UInputAction>(this)};
	DirectorMappingContext->MapKey(ButtonAction, Key);
	return ButtonAction;
}

void ADirectorCharacter::MapMoveKey(const FKey& Key,
	bool bSwapAxes, bool bNegate)
{
	FEnhancedActionKeyMapping& Mapping{
		DirectorMappingContext->MapKey(MoveAction, Key)};
	if (bSwapAxes)
	{
		Mapping.Modifiers.Add(
			NewObject<UInputModifierSwizzleAxis>(this));
	}
	if (bNegate)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}
}

void ADirectorCharacter::AddMappingContext() const
{
	const APlayerController* PlayerController{
		Cast<APlayerController>(GetController())};
	if (!PlayerController)
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem{
		ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(
				PlayerController->GetLocalPlayer())};
	if (!InputSubsystem)
	{
		return;
	}
	InputSubsystem->ClearAllMappings();
	InputSubsystem->AddMappingContext(DirectorMappingContext, 0);
}

void ADirectorCharacter::BindInputActions(
	UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInput{
		CastChecked<UEnhancedInputComponent>(PlayerInputComponent)};
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered,
		this, &ADirectorCharacter::HandleMove);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered,
		this, &ADirectorCharacter::HandleLook);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started,
		this, &ADirectorCharacter::HandleJumpPressed);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed,
		this, &ADirectorCharacter::HandleJumpReleased);
	EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started,
		this, &ADirectorCharacter::HandleCrouchPressed);
	EnhancedInput->BindAction(InteractAction,
		ETriggerEvent::Started, this,
		&ADirectorCharacter::HandleInteractPressed);
	EnhancedInput->BindAction(FlashAction, ETriggerEvent::Started,
		this, &ADirectorCharacter::HandleFlashPressed);
	EnhancedInput->BindAction(TakedownAction,
		ETriggerEvent::Started, this,
		&ADirectorCharacter::HandleTakedownPressed);
	EnhancedInput->BindAction(MegaphoneAction,
		ETriggerEvent::Started, this,
		&ADirectorCharacter::HandleMegaphonePressed);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Started,
		this, &ADirectorCharacter::HandleFirePressed);
}

void ADirectorCharacter::HandleMove(
	const FInputActionValue& ActionValue)
{
	const FVector2D MoveInput{ActionValue.Get<FVector2D>()};
	const FRotator YawRotation{0.0f, GetControlRotation().Yaw, 0.0f};
	const FRotationMatrix YawMatrix{YawRotation};
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), MoveInput.X);
}

void ADirectorCharacter::HandleLook(
	const FInputActionValue& ActionValue)
{
	const FVector2D LookInput{ActionValue.Get<FVector2D>()};
	const float LookSensitivity{
		UBacklotTuningSettings::Get().LookSensitivity};
	AddControllerYawInput(LookInput.X * LookSensitivity);
	AddControllerPitchInput(-LookInput.Y * LookSensitivity);
}

void ADirectorCharacter::HandleJumpPressed()
{
	UnCrouch();
	Jump();
}

void ADirectorCharacter::HandleJumpReleased()
{
	StopJumping();
}

void ADirectorCharacter::HandleCrouchPressed()
{
	if (IsCrouched())
	{
		UnCrouch();
		return;
	}
	Crouch();
}

void ADirectorCharacter::HandleInteractPressed()
{
	if (MountedCameraRig.IsValid())
	{
		DismountCameraRig();
		return;
	}
	if (TryUseUploadTerminal() || TryMountCameraRig()
		|| TryReelInStunnedGuard())
	{
		return;
	}
	Grapple->TryStartGrapple();
}

bool ADirectorCharacter::TryReelInStunnedGuard()
{
	const AGuardCharacter* Guard{Grapple->FindPullableGuard()};
	if (!Guard)
	{
		return false;
	}
	const FVector GuardLocation{Guard->GetActorLocation()};
	Grapple->TryPullStunnedGuard();
	Takedown->PlayCutFeedback(GuardLocation);
	return true;
}

bool ADirectorCharacter::TryMountCameraRig()
{
	for (TActorIterator<ACameraRigTurret> RigIterator{GetWorld()};
		RigIterator; ++RigIterator)
	{
		if (!RigIterator->CanBeMountedFrom(GetActorLocation()))
		{
			continue;
		}
		MountedCameraRig = *RigIterator;
		Grapple->CancelGrapple();
		UnCrouch();
		SetActorLocation(RigIterator->GetSeatLocation());
		GetCharacterMovement()->DisableMovement();
		bUseControllerRotationYaw = true;
		return true;
	}
	return false;
}

void ADirectorCharacter::DismountCameraRig()
{
	MountedCameraRig.Reset();
	bUseControllerRotationYaw = false;
	if (GetCharacterMovement()->MovementMode == MOVE_None)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void ADirectorCharacter::HandleFirePressed()
{
	if (!MountedCameraRig.IsValid() || !GetController())
	{
		return;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	GetController()->GetPlayerViewPoint(ViewLocation,
		ViewRotation);
	MountedCameraRig->TryFireSpotlight(ViewLocation,
		ViewRotation.Vector());
}

void ADirectorCharacter::HandleMegaphonePressed()
{
	ADirectorPlayerController* DirectorController{
		Cast<ADirectorPlayerController>(GetController())};
	if (!Megaphone->TryMakeNoise() || !DirectorController
		|| !DirectorController->GetBacklotHUD())
	{
		return;
	}
	DirectorController->GetBacklotHUD()->ShowWorldPopup(
		TEXT("HEY! OVER HERE!"),
		Megaphone->GetLastNoiseLocation(),
		FLinearColor{0.4f, 0.8f, 1.0f});
}

bool ADirectorCharacter::TryUseUploadTerminal()
{
	for (TActorIterator<AUploadTerminal> TerminalIterator{
			GetWorld()};
		TerminalIterator; ++TerminalIterator)
	{
		if (TerminalIterator->CanBeUsedFrom(GetActorLocation()))
		{
			TerminalIterator->StartUpload();
			return true;
		}
	}
	return false;
}

void ADirectorCharacter::HandleFlashPressed()
{
	if (!CameraFlash->TryFlash())
	{
		return;
	}
	FlashLight->SetIntensity(
		DirectorConstants::FlashLightIntensity);
	GetWorldTimerManager().SetTimer(FlashLightTimerHandle, this,
		&ADirectorCharacter::HideFlashLight,
		DirectorConstants::FlashLightSeconds);
}

void ADirectorCharacter::HideFlashLight()
{
	FlashLight->SetIntensity(0.0f);
}

void ADirectorCharacter::HandleTakedownPressed()
{
	Takedown->TryTakedown();
}

void ADirectorCharacter::OnStartCrouch(float HalfHeightAdjust,
	float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	GetMesh()->SetRelativeScale3D(FVector{1.0f, 1.0f,
		DirectorConstants::CrouchedMeshHeightScale});
}

void ADirectorCharacter::OnEndCrouch(float HalfHeightAdjust,
	float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	GetMesh()->SetRelativeScale3D(FVector::OneVector);
}

void ADirectorCharacter::FellOutOfWorld(
	const UDamageType& DamageType)
{
	HandleHealthDepleted();
}

void ADirectorCharacter::ReceiveGuardShot(const FVector& ShotOrigin)
{
	if (!Health->ApplyHit())
	{
		return;
	}
	const FVector AwayDirection{
		(GetActorLocation() - ShotOrigin).GetSafeNormal2D()};
	if (Health->GetCurrentHealth() > 0 && !Grapple->IsGrappling()
		&& !MountedCameraRig.IsValid())
	{
		LaunchCharacter(AwayDirection
			* DirectorConstants::ShotKnockbackSpeed, false, false);
	}
}

void ADirectorCharacter::HandleHealthDepleted()
{
	ABacklotGameMode* GameMode{
		GetWorld()->GetAuthGameMode<ABacklotGameMode>()};
	if (GameMode)
	{
		GameMode->HandlePlayerCaught();
	}
}

void ADirectorCharacter::RespawnAt(
	const FTransform& RespawnTransform)
{
	DismountCameraRig();
	Grapple->CancelGrapple();
	UnCrouch();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorLocation(RespawnTransform.GetLocation(), false,
		nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(RespawnTransform.Rotator());
	if (AController* PlayerController{GetController()})
	{
		PlayerController->SetControlRotation(
			RespawnTransform.Rotator());
	}
	Health->RestoreFullHealth();
}

UDirectorHealthComponent*
ADirectorCharacter::GetHealthComponent() const
{
	return Health;
}

UGrappleComponent* ADirectorCharacter::GetGrappleComponent() const
{
	return Grapple;
}

UCameraFlashComponent*
ADirectorCharacter::GetCameraFlashComponent() const
{
	return CameraFlash;
}

UTakedownComponent*
ADirectorCharacter::GetTakedownComponent() const
{
	return Takedown;
}

UMegaphoneComponent*
ADirectorCharacter::GetMegaphoneComponent() const
{
	return Megaphone;
}

bool ADirectorCharacter::IsMountedOnCameraRig() const
{
	return MountedCameraRig.IsValid();
}
