#include "Character/DirectorCharacter.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Component/GrappleComponent.h"
#include "Component/TakedownComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ClapGrip{TEXT("grip_clap")};
	const FName ReelGrip{TEXT("grip_reel")};

	/* Enlarge the props for the gameplay camera while retaining their grip */
	constexpr float ClapboardScale{1.55f};
	constexpr float ReelScale{1.45f};
	const FVector ClapboardGripOffset{6.55f, 0.0f, 0.0f};
	const FVector ReelGripOffset{0.0f, 2.93f, 1.22f};

	/* Time of the hinge closure in the source sweep clip */
	constexpr float SweepContactSeconds{33.0f / 30.0f};

	template <typename AssetType>
	AssetType* FindAsset(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinderOptional<AssetType> Finder{Path};
		return Finder.Get();
	}
}

void ADirectorCharacter::CreatePresentationComponents()
{
	Clapboard = CreateDefaultSubobject<USkeletalMeshComponent>(
		TEXT("Clapboard"));
	Clapboard->SetupAttachment(GetMesh(), ClapGrip);
	Clapboard->SetRelativeScale3D(FVector{ClapboardScale});
	Clapboard->SetRelativeLocation(ClapboardGripOffset);
	Clapboard->SetSkeletalMesh(FindAsset<USkeletalMesh>(TEXT(
		"/Game/Characters/Director/Props/Clapboard/SK_Clapboard")));

	FilmReel = CreateDefaultSubobject<USkeletalMeshComponent>(
		TEXT("FilmReel"));
	FilmReel->SetupAttachment(GetMesh(), ReelGrip);
	FilmReel->SetRelativeScale3D(FVector{ReelScale});
	FilmReel->SetRelativeLocation(ReelGripOffset);
	FilmReel->SetSkeletalMesh(FindAsset<USkeletalMesh>(TEXT(
		"/Game/Characters/Director/Props/FilmReel/SK_FilmReel")));

	for (USkeletalMeshComponent* Prop : {Clapboard.Get(), FilmReel.Get()})
	{
		Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Prop->SetCanEverAffectNavigation(false);
		Prop->SetHiddenInGame(true);
	}

	DirectorMesh = FindAsset<USkeletalMesh>(TEXT(
		"/Game/Characters/Director/Character/SK_Director"));
	StandingAnimation = FindAsset<UBlendSpace>(TEXT(
		"/Game/Characters/Director/Locomotion/BS_Director_Standing"));
	CrouchAnimation = FindAsset<UBlendSpace>(TEXT(
		"/Game/Characters/Director/Locomotion/BS_Director_Crouched"));
	FallingAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Locomotion/AS_Director_JumpLoop"));
	QuickCutAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Actions/AS_Director_QuickCut"));
	SweepCutAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Actions/AS_Director_SweepCut"));
	GrappleAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Actions/AS_Director_Grapple"));
	QuickClapAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Props/Clapboard/AS_Clapboard_QuickCut"));
	SweepClapAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Props/Clapboard/AS_Clapboard_SweepCut"));
	ReelAnimation = FindAsset<UAnimSequence>(TEXT(
		"/Game/Characters/Director/Props/FilmReel/AS_FilmReel_Extend"));

	if (DirectorMesh && StandingAnimation && CrouchAnimation)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->SetSkeletalMesh(DirectorMesh);
		GetMesh()->OverrideAnimationData(StandingAnimation);
	}
}

void ADirectorCharacter::StartGesture(UAnimSequence* BodyAnimation,
	USkeletalMeshComponent* Prop, UAnimSequence* PropAnimation,
	float PlayRate)
{
	if (!bUsesDirectorPresentation || !BodyAnimation || !PropAnimation
		|| !Prop->GetSkeletalMeshAsset())
	{
		return;
	}

	ResetGesture();
	ActiveGesture = BodyAnimation;
	GetMesh()->PlayAnimation(BodyAnimation, false);
	GetMesh()->SetPlayRate(PlayRate);
	Prop->PlayAnimation(PropAnimation, false);
	Prop->SetPlayRate(PlayRate);
	if (UAnimSingleNodeInstance* Instance{Prop->GetSingleNodeInstance()})
	{
		Instance->SetReverse(false);
	}
	Prop->SetHiddenInGame(false);
}

void ADirectorCharacter::PlayClapboard(bool bSweeping)
{
	const float StruggleSeconds{FMath::Max(0.01f,
		UBacklotTuningSettings::Get().AlertTakedownSeconds)};
	const float PlayRate{bSweeping
		? SweepContactSeconds / StruggleSeconds : 1.0f};
	StartGesture(bSweeping ? SweepCutAnimation.Get() : QuickCutAnimation.Get(),
		Clapboard, bSweeping ? SweepClapAnimation.Get()
			: QuickClapAnimation.Get(),
		PlayRate);
}

void ADirectorCharacter::ResetGesture()
{
	ActiveGesture = nullptr;
	Clapboard->SetHiddenInGame(true);
	FilmReel->SetHiddenInGame(true);
}

void ADirectorCharacter::UpdateDirectorAnimation()
{
	if (!bUsesDirectorPresentation)
	{
		return;
	}

	USkeletalMeshComponent* Body{GetMesh()};
	if (ActiveGesture)
	{
		const bool bGrapple{ActiveGesture == GrappleAnimation};
		if (bGrapple && !Grapple->IsGrappling())
		{
			ActiveGesture = nullptr;
			if (UAnimSingleNodeInstance* Reel{FilmReel->GetSingleNodeInstance()})
			{
				Reel->SetReverse(true);
				Reel->SetPlaying(FilmReel->GetPosition() > 0.0f);
			}
		}
		else
		{
			const bool bSweepCanceled{ActiveGesture == SweepCutAnimation
				&& !Takedown->IsStruggling()
				&& Body->GetPosition() < SweepContactSeconds - 0.10f};
			if (!bSweepCanceled && (Body->IsPlaying() || bGrapple))
			{
				return;
			}
			ResetGesture();
		}
	}

	if (!FilmReel->bHiddenInGame && !FilmReel->IsPlaying())
	{
		FilmReel->SetHiddenInGame(true);
	}

	const UCharacterMovementComponent* Movement{GetCharacterMovement()};
	UAnimationAsset* Animation{bIsCrouched ? CrouchAnimation.Get()
		: StandingAnimation.Get()};
	if (Movement->IsFalling() && FallingAnimation)
	{
		Animation = FallingAnimation;
	}
	UAnimSingleNodeInstance* Instance{Body->GetSingleNodeInstance()};
	if (!Instance || Instance->GetAnimationAsset() != Animation)
	{
		Body->PlayAnimation(Animation, true);
		Body->SetPlayRate(1.0f);
		Instance = Body->GetSingleNodeInstance();
	}
	if (Instance && Animation != FallingAnimation)
	{
		const UBacklotTuningSettings& Tuning{UBacklotTuningSettings::Get()};
		const FVector Velocity{GetActorTransform()
			.InverseTransformVectorNoScale(GetVelocity())};
		const float Speed{static_cast<float>(
			FVector2D{Velocity.X, Velocity.Y}.Size())};
		const float Range{bIsCrouched ? Tuning.PlayerCrouchSpeed
			: Tuning.PlayerSprintSpeed};
		const float Weight{FMath::Clamp(Speed / FMath::Max(Range, 1.0f),
			0.0f, 1.0f)};
		Instance->SetBlendSpacePosition(Velocity.GetSafeNormal2D() * Weight);
	}
}

