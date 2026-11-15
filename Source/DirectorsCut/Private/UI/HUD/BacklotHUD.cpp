#include "UI/HUD/BacklotHUD.h"

#include "Actor/CameraRigTurret.h"
#include "Actor/GrappleHookPoint.h"
#include "Actor/UploadTerminal.h"
#include "Character/DirectorCharacter.h"
#include "Character/GuardCharacter.h"
#include "Component/CameraFlashComponent.h"
#include "Component/DirectorHealthComponent.h"
#include "Component/GrappleComponent.h"
#include "Component/MegaphoneComponent.h"
#include "Component/TakedownComponent.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Game/BacklotGameState.h"

namespace BacklotHUDConstants
{
	/* Distance from the screen edge to the corner elements */
	constexpr float ScreenMargin{40.0f};

	/* Guards farther than this do not show a meter */
	constexpr float GuardMeterMaximumDistance{4000.0f};

	/* Seconds a world popup stays up, and how far it rises */
	constexpr float PopupSeconds{1.3f};
	constexpr float PopupRiseDistance{90.0f};

	/* Seconds the center message stays up */
	constexpr float CenterMessageSeconds{3.0f};

	/* Seconds the white flash and red hit tint last */
	constexpr float FlashWhiteoutSeconds{0.3f};
	constexpr float HitTintSeconds{0.35f};

	const FLinearColor BarBackColor{0.0f, 0.0f, 0.0f, 0.6f};
	const FLinearColor TextColor{1.0f, 1.0f, 1.0f};
	const FLinearColor GoodColor{0.3f, 1.0f, 0.4f};
	const FLinearColor WarnColor{1.0f, 0.85f, 0.1f};
	const FLinearColor BadColor{1.0f, 0.2f, 0.2f};

	/* Lines of the controls list */
	const TArray<FString> ControlsLines{
		TEXT("CONTROLS"),
		TEXT("WASD  Move        Mouse  Look"),
		TEXT("Space  Jump"),
		TEXT("C / Ctrl  Crouch (quiet, harder to see)"),
		TEXT("E  Film reel grapple to the yellow point"),
		TEXT("Q  Camera flash (stuns guards in front)"),
		TEXT("F  Clapperboard takedown (stunned,"),
		TEXT("    unaware from behind, or from above)"),
		TEXT("E on a stunned guard  Reel him in"),
		TEXT("G  Megaphone (lures guards to where you aim)"),
		TEXT("Guard shots: sidestep when the laser turns red"),
		TEXT("F1  Toggle guard vision cones")};

	/* Returns the label shown above a guard for its state */
	FString GetGuardStateLabel(EGuardState GuardState)
	{
		switch (GuardState)
		{
		case EGuardState::Suspicious:
			return TEXT("?");
		case EGuardState::Alert:
			return TEXT("!");
		case EGuardState::Search:
			return TEXT("SEARCHING");
		case EGuardState::Stunned:
			return TEXT("STUNNED");
		default:
			return FString{};
		}
	}
}

ABacklotHUD::ABacklotHUD()
	: CenterMessageStartTimeSeconds{-1000.0f}
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABacklotHUD::DrawHUD()
{
	Super::DrawHUD();
	const ADirectorCharacter* Director{GetDirector()};
	const ABacklotGameState* GameState{
		GetWorld()->GetGameState<ABacklotGameState>()};
	if (!Canvas || !Director || !GameState)
	{
		return;
	}
	if (GameState->IsGameWon())
	{
		DrawWinScreen();
		return;
	}
	DrawGuardMeters();
	DrawGrappleMarker(*Director);
	DrawReelInPrompt(*Director);
	DrawTakedownPrompt(*Director);
	DrawWorldPopups();
	DrawScreenFlashes(*Director);
	DrawHealth(*Director);
	DrawFlashCooldown(*Director);
	DrawMegaphoneCooldown(*Director);
	DrawZoneName(*GameState);
	DrawUpload(*GameState, *Director);
	DrawCameraRigPrompt(*Director);
	DrawCrosshair();
	DrawCenterMessage();
	if (GameState->IsControlsHelpVisible())
	{
		DrawControlsHelp();
	}
}

void ABacklotHUD::DrawGuardMeters()
{
	for (TActorIterator<AGuardCharacter> GuardIterator{GetWorld()};
		GuardIterator; ++GuardIterator)
	{
		if (!GuardIterator->IsTakenDown())
		{
			DrawGuardMeter(**GuardIterator);
		}
	}
}

void ABacklotHUD::DrawGuardMeter(const AGuardCharacter& Guard)
{
	using namespace BacklotHUDConstants;
	const FVector HeadLocation{Guard.GetActorLocation()
		+ FVector{0.0f, 0.0f, 150.0f}};
	const ADirectorCharacter* Director{GetDirector()};
	FVector2D ScreenPosition;
	const bool bIsTooFar{FVector::Dist(HeadLocation,
		Director->GetActorLocation()) > GuardMeterMaximumDistance};
	if (bIsTooFar || !ProjectToScreen(HeadLocation, ScreenPosition))
	{
		return;
	}
	const FVector2D MeterSize{70.0f, 9.0f};
	const float Meter{Guard.GetDetectionMeter()};
	const bool bIsCalm{Meter <= 0.0f
		&& Guard.GetGuardState() == EGuardState::Patrol};
	if (bIsCalm)
	{
		return;
	}
	const FLinearColor MeterColor{
		Guard.IsAlert() ? BadColor : WarnColor};
	DrawBar(ScreenPosition - FVector2D{MeterSize.X * 0.5f, 0.0f},
		MeterSize, Guard.IsAlert() ? 1.0f : Meter, MeterColor);
	DrawCenteredLabel(GetGuardStateLabel(Guard.GetGuardState()),
		ScreenPosition - FVector2D{0.0f, 22.0f}, MeterColor, 1.6f);
}

void ABacklotHUD::DrawGrappleMarker(
	const ADirectorCharacter& Director)
{
	const AGrappleHookPoint* HookPoint{
		Director.GetGrappleComponent()->GetTargetedHookPoint()};
	FVector2D ScreenPosition;
	if (!HookPoint || !ProjectToScreen(
		HookPoint->GetActorLocation(), ScreenPosition))
	{
		return;
	}
	const float HalfSize{26.0f};
	const float LineThickness{3.0f};
	const FLinearColor MarkerColor{
		BacklotHUDConstants::WarnColor};
	const FVector2D TopLeft{
		ScreenPosition - FVector2D{HalfSize, HalfSize}};
	const float FullSize{HalfSize * 2.0f};
	DrawRect(MarkerColor, TopLeft.X, TopLeft.Y, FullSize,
		LineThickness);
	DrawRect(MarkerColor, TopLeft.X, TopLeft.Y + FullSize,
		FullSize + LineThickness, LineThickness);
	DrawRect(MarkerColor, TopLeft.X, TopLeft.Y, LineThickness,
		FullSize);
	DrawRect(MarkerColor, TopLeft.X + FullSize, TopLeft.Y,
		LineThickness, FullSize);
	DrawCenteredLabel(TEXT("[E] GRAPPLE"),
		ScreenPosition + FVector2D{0.0f, HalfSize + 14.0f},
		MarkerColor, 1.2f);
}

void ABacklotHUD::DrawReelInPrompt(
	const ADirectorCharacter& Director)
{
	const AGuardCharacter* Guard{
		Director.GetGrappleComponent()->FindPullableGuard()};
	FVector2D ScreenPosition;
	if (!Guard || !ProjectToScreen(Guard->GetActorLocation()
		+ FVector{0.0f, 0.0f, 60.0f}, ScreenPosition))
	{
		return;
	}
	DrawCenteredLabel(TEXT("[E] REEL HIM IN"), ScreenPosition,
		BacklotHUDConstants::WarnColor, 1.6f);
}

void ABacklotHUD::DrawTakedownPrompt(
	const ADirectorCharacter& Director)
{
	const FTakedownOpportunity Opportunity{
		Director.GetTakedownComponent()
			->FindTakedownOpportunity()};
	FVector2D ScreenPosition;
	if (!Opportunity.Guard.IsValid() || !ProjectToScreen(
		Opportunity.Guard->GetActorLocation(), ScreenPosition))
	{
		return;
	}
	if (Opportunity.bWillSucceed)
	{
		DrawCenteredLabel(TEXT("[F] CUT!"), ScreenPosition,
			BacklotHUDConstants::GoodColor, 2.0f);
		return;
	}
	DrawCenteredLabel(TEXT("[F] too risky - he sees you"),
		ScreenPosition, BacklotHUDConstants::BadColor, 1.4f);
}

void ABacklotHUD::DrawWorldPopups()
{
	using namespace BacklotHUDConstants;
	const double CurrentTimeSeconds{GetWorld()->GetTimeSeconds()};
	WorldPopups.RemoveAll(
		[CurrentTimeSeconds](const FWorldPopup& Popup)
		{
			return CurrentTimeSeconds - Popup.StartTimeSeconds
				> PopupSeconds;
		});
	for (const FWorldPopup& Popup : WorldPopups)
	{
		const double Age{(CurrentTimeSeconds
			- Popup.StartTimeSeconds) / PopupSeconds};
		FVector2D ScreenPosition;
		if (ProjectToScreen(Popup.WorldLocation, ScreenPosition))
		{
			DrawCenteredLabel(Popup.Text, ScreenPosition
				- FVector2D{0.0f, Age * PopupRiseDistance},
				Popup.Color, 3.5f);
		}
	}
}

void ABacklotHUD::DrawHealth(const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const UDirectorHealthComponent* Health{
		Director.GetHealthComponent()};
	const FVector2D PipSize{46.0f, 20.0f};
	const float PipSpacing{8.0f};
	DrawLabel(TEXT("HEALTH"),
		FVector2D{ScreenMargin, ScreenMargin}, TextColor, 1.2f);
	for (int32 PipIndex{0}; PipIndex < Health->GetMaximumHealth();
		++PipIndex)
	{
		const bool bIsFilled{
			PipIndex < Health->GetCurrentHealth()};
		DrawRect(bIsFilled ? BadColor : BarBackColor,
			ScreenMargin + PipIndex * (PipSize.X + PipSpacing),
			ScreenMargin + 24.0f, PipSize.X, PipSize.Y);
	}
}

void ABacklotHUD::DrawFlashCooldown(
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const float CooldownRemaining{
		Director.GetCameraFlashComponent()
			->GetCooldownFractionRemaining()};
	const bool bIsReady{CooldownRemaining <= 0.0f};
	const FVector2D LabelPosition{
		ScreenMargin, ScreenMargin + 58.0f};
	DrawLabel(bIsReady ? TEXT("FLASH [Q]  READY")
		: TEXT("FLASH [Q]  recharging"),
		LabelPosition, bIsReady ? GoodColor : TextColor, 1.2f);
	DrawBar(LabelPosition + FVector2D{0.0f, 24.0f},
		FVector2D{154.0f, 12.0f}, 1.0f - CooldownRemaining,
		bIsReady ? GoodColor : WarnColor);
}

void ABacklotHUD::DrawMegaphoneCooldown(
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const float CooldownRemaining{
		Director.GetMegaphoneComponent()
			->GetCooldownFractionRemaining()};
	const bool bIsReady{CooldownRemaining <= 0.0f};
	const FVector2D LabelPosition{
		ScreenMargin, ScreenMargin + 104.0f};
	DrawLabel(bIsReady ? TEXT("MEGAPHONE [G]  READY")
		: TEXT("MEGAPHONE [G]  recharging"),
		LabelPosition, bIsReady ? GoodColor : TextColor, 1.2f);
	DrawBar(LabelPosition + FVector2D{0.0f, 24.0f},
		FVector2D{154.0f, 12.0f}, 1.0f - CooldownRemaining,
		bIsReady ? GoodColor : WarnColor);
}

void ABacklotHUD::DrawScreenFlashes(
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const float SecondsSinceFlash{
		Director.GetCameraFlashComponent()
			->GetSecondsSinceLastFlash()};
	if (SecondsSinceFlash < FlashWhiteoutSeconds)
	{
		const float Opacity{0.8f
			* (1.0f - SecondsSinceFlash / FlashWhiteoutSeconds)};
		DrawRect(FLinearColor{1.0f, 1.0f, 1.0f, Opacity}, 0.0f,
			0.0f, Canvas->SizeX, Canvas->SizeY);
	}
	const float SecondsSinceHit{
		Director.GetHealthComponent()->GetSecondsSinceLastHit()};
	if (SecondsSinceHit < HitTintSeconds)
	{
		const float Opacity{0.45f
			* (1.0f - SecondsSinceHit / HitTintSeconds)};
		DrawRect(FLinearColor{1.0f, 0.0f, 0.0f, Opacity}, 0.0f,
			0.0f, Canvas->SizeX, Canvas->SizeY);
	}
}

void ABacklotHUD::DrawZoneName(const ABacklotGameState& GameState)
{
	DrawCenteredLabel(GameState.GetCurrentZoneName().ToUpper(),
		FVector2D{Canvas->SizeX * 0.5f,
			BacklotHUDConstants::ScreenMargin + 10.0f},
		BacklotHUDConstants::TextColor, 2.0f);
}

void ABacklotHUD::DrawUpload(const ABacklotGameState& GameState,
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const FVector2D BarSize{520.0f, 26.0f};
	const FVector2D BarPosition{
		Canvas->SizeX * 0.5f - BarSize.X * 0.5f,
		ScreenMargin + 70.0f};
	if (GameState.IsUploadActive())
	{
		const bool bIsPaused{GameState.IsUploadPaused()};
		const float Progress{GameState.GetUploadProgress()};
		DrawBar(BarPosition, BarSize, Progress,
			bIsPaused ? BadColor : GoodColor);
		DrawCenteredLabel(bIsPaused
			? TEXT("UPLOAD PAUSED - GET BACK TO THE TERMINAL")
			: FString::Printf(TEXT("UPLOADING THE FILM  %d%%"),
				FMath::FloorToInt32(Progress * 100.0f)),
			BarPosition + FVector2D{BarSize.X * 0.5f, -16.0f},
			bIsPaused ? BadColor : TextColor, 1.4f);
		return;
	}
	for (TActorIterator<AUploadTerminal> TerminalIterator{
			GetWorld()};
		TerminalIterator; ++TerminalIterator)
	{
		if (TerminalIterator->CanBeUsedFrom(
			Director.GetActorLocation()))
		{
			DrawCenteredLabel(TEXT("[E] START THE UPLOAD"),
				FVector2D{Canvas->SizeX * 0.5f,
					Canvas->SizeY * 0.62f}, GoodColor, 2.2f);
		}
	}
}

void ABacklotHUD::DrawCameraRigPrompt(
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const FVector2D PromptCenter{
		Canvas->SizeX * 0.5f, Canvas->SizeY * 0.72f};
	if (Director.IsMountedOnCameraRig())
	{
		DrawCenteredLabel(TEXT("CAMERA RIG   Left mouse: "
			"spotlight stun    E: get off"),
			PromptCenter, WarnColor, 1.6f);
		return;
	}
	for (TActorIterator<ACameraRigTurret> RigIterator{GetWorld()};
		RigIterator; ++RigIterator)
	{
		if (RigIterator->CanBeMountedFrom(
			Director.GetActorLocation()))
		{
			DrawCenteredLabel(TEXT("[E] MOUNT THE CAMERA RIG"),
				PromptCenter, GoodColor, 1.8f);
		}
	}
}

void ABacklotHUD::DrawControlsHelp()
{
	using namespace BacklotHUDConstants;
	const float LineHeight{24.0f};
	const float PanelWidth{470.0f};
	const FVector2D PanelPosition{
		Canvas->SizeX - PanelWidth - ScreenMargin, ScreenMargin};
	DrawRect(BarBackColor, PanelPosition.X - 12.0f,
		PanelPosition.Y - 8.0f, PanelWidth + 24.0f,
		ControlsLines.Num() * LineHeight + 16.0f);
	for (int32 LineIndex{0}; LineIndex < ControlsLines.Num();
		++LineIndex)
	{
		DrawLabel(ControlsLines[LineIndex], PanelPosition
			+ FVector2D{0.0f, LineIndex * LineHeight},
			LineIndex == 0 ? WarnColor : TextColor, 1.2f);
	}
}

void ABacklotHUD::DrawCenterMessage()
{
	const double MessageAge{GetWorld()->GetTimeSeconds()
		- CenterMessageStartTimeSeconds};
	if (MessageAge > BacklotHUDConstants::CenterMessageSeconds)
	{
		return;
	}
	DrawCenteredLabel(CenterMessage,
		FVector2D{Canvas->SizeX * 0.5f, Canvas->SizeY * 0.3f},
		BacklotHUDConstants::BadColor, 2.6f);
}

void ABacklotHUD::DrawWinScreen()
{
	using namespace BacklotHUDConstants;
	DrawRect(FLinearColor{0.0f, 0.0f, 0.0f, 0.75f}, 0.0f, 0.0f,
		Canvas->SizeX, Canvas->SizeY);
	const FVector2D ScreenCenter{
		Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f};
	DrawCenteredLabel(TEXT("THAT'S A WRAP!"),
		ScreenCenter - FVector2D{0.0f, 80.0f}, WarnColor, 6.0f);
	DrawCenteredLabel(
		TEXT("The director's cut is uploaded. You win."),
		ScreenCenter + FVector2D{0.0f, 10.0f}, TextColor, 2.2f);
	DrawCenteredLabel(TEXT("Press R to restart"),
		ScreenCenter + FVector2D{0.0f, 70.0f}, GoodColor, 2.2f);
}

void ABacklotHUD::DrawCrosshair()
{
	const float DotSize{4.0f};
	DrawRect(FLinearColor{1.0f, 1.0f, 1.0f, 0.7f},
		Canvas->SizeX * 0.5f - DotSize * 0.5f,
		Canvas->SizeY * 0.5f - DotSize * 0.5f, DotSize, DotSize);
}

void ABacklotHUD::DrawLabel(const FString& Text,
	const FVector2D& Position, const FLinearColor& Color,
	float Scale)
{
	FCanvasTextItem TextItem{Position, FText::FromString(Text),
		GEngine->GetMediumFont(), Color};
	TextItem.Scale = FVector2D{Scale, Scale};
	TextItem.bOutlined = true;
	TextItem.OutlineColor = FLinearColor::Black;
	Canvas->DrawItem(TextItem);
}

void ABacklotHUD::DrawCenteredLabel(const FString& Text,
	const FVector2D& Center, const FLinearColor& Color,
	float Scale)
{
	if (Text.IsEmpty())
	{
		return;
	}
	float TextWidth{0.0f};
	float TextHeight{0.0f};
	GetTextSize(Text, TextWidth, TextHeight,
		GEngine->GetMediumFont(), Scale);
	DrawLabel(Text, Center
		- FVector2D{TextWidth * 0.5f, TextHeight * 0.5f},
		Color, Scale);
}

void ABacklotHUD::DrawBar(const FVector2D& Position,
	const FVector2D& Size, float FilledFraction,
	const FLinearColor& FillColor)
{
	DrawRect(BacklotHUDConstants::BarBackColor, Position.X,
		Position.Y, Size.X, Size.Y);
	DrawRect(FillColor, Position.X, Position.Y,
		Size.X * FMath::Clamp(FilledFraction, 0.0f, 1.0f),
		Size.Y);
}

bool ABacklotHUD::ProjectToScreen(const FVector& WorldLocation,
	FVector2D& OutScreenPosition) const
{
	const FVector Projected{Project(WorldLocation, true)};
	OutScreenPosition = FVector2D{Projected.X, Projected.Y};
	return Projected.Z > 0.0f;
}

void ABacklotHUD::ShowWorldPopup(const FString& Text,
	const FVector& WorldLocation, const FLinearColor& Color)
{
	FWorldPopup Popup;
	Popup.Text = Text;
	Popup.WorldLocation = WorldLocation;
	Popup.Color = Color;
	Popup.StartTimeSeconds = GetWorld()->GetTimeSeconds();
	WorldPopups.Add(Popup);
}

void ABacklotHUD::ShowCenterMessage(const FString& Message)
{
	CenterMessage = Message;
	CenterMessageStartTimeSeconds = GetWorld()->GetTimeSeconds();
}

ADirectorCharacter* ABacklotHUD::GetDirector() const
{
	return Cast<ADirectorCharacter>(GetOwningPawn());
}
