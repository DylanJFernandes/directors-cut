#include "UI/HUD/BacklotHUD.h"

#include "Actor/GrappleHookPoint.h"
#include "Actor/UploadTerminal.h"
#include "Character/DirectorCharacter.h"
#include "Character/GuardCharacter.h"
#include "Component/CameraFlashComponent.h"
#include "Component/DirectorHealthComponent.h"
#include "Component/GrappleComponent.h"
#include "Component/MegaphoneComponent.h"
#include "Component/TakedownComponent.h"
#include "Component/TommyGunComponent.h"
#include "Data/BacklotTuningSettings.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Game/BacklotGameState.h"

namespace BacklotHUDConstants
{
	/* Distance from the screen edge to the corner elements */
	constexpr float ScreenMargin{24.0f};
	constexpr float StatusWidth{234.0f};
	constexpr float StatusHeight{176.0f};
	constexpr float PanelInset{14.0f};
	constexpr float LabelScale{0.85f};

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

	/* Seconds a megaphone ring takes to reach its full size,
	   and the number of straight pieces it is drawn with */
	constexpr float MegaphoneRingSeconds{0.9f};
	constexpr int32 RingSegmentCount{40};

	/* Seconds the tommy gun hit marker stays up */
	constexpr float HitMarkerSeconds{0.08f};

	const FLinearColor PanelColor{0.032f, 0.030f, 0.034f, 0.96f};
	const FLinearColor BarBackColor{0.10f, 0.087f, 0.075f, 0.95f};
	const FLinearColor RuleColor{0.39f, 0.30f, 0.19f};
	const FLinearColor TextColor{0.95f, 0.89f, 0.75f};
	const FLinearColor MutedColor{0.73f, 0.67f, 0.56f};
	const FLinearColor GoodColor{0.71f, 0.81f, 0.69f};
	const FLinearColor WarnColor{0.87f, 0.68f, 0.42f};
	const FLinearColor BadColor{0.96f, 0.47f, 0.39f};

	/* Lines of the controls list */
	const TArray<FString> ControlsLines{
		TEXT("CONTROLS"),
		TEXT("WASD  Move        Mouse  Look"),
		TEXT("Shift  Sprint (guards hear it)    Space  Jump"),
		TEXT("C / Ctrl  Crouch (quiet, harder to see)"),
		TEXT("C while sprinting or landing  Slide"),
		TEXT("    jump out of a slide to keep the speed"),
		TEXT("E  Film reel grapple to the yellow point"),
		TEXT("E aimed at a guard  Zip to him"),
		TEXT("Q  Camera flash (stuns guards in front)"),
		TEXT("F  Clapperboard takedown: instant if he is"),
		TEXT("    stunned or unaware, slow if he sees you"),
		TEXT("G  Megaphone (guards come to where you are)"),
		TEXT("Guard shots: keep moving when the laser is red"),
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
		DrawWinScreen(*GameState);
		return;
	}
	if (GameState->IsGameLost())
	{
		DrawLoseScreen();
		return;
	}
	DrawGuardMeters();
	DrawGrappleMarker(*Director);
	DrawGuardGrapplePrompt(*Director);
	DrawMegaphoneRings(*Director);
	DrawTakedownPrompt(*Director);
	DrawWorldPopups();
	DrawScreenFlashes(*Director);
	const FVector2D StatusPosition{BacklotHUDConstants::ScreenMargin,
		BacklotHUDConstants::ScreenMargin};
	DrawDecoPanel(StatusPosition,
		FVector2D{BacklotHUDConstants::StatusWidth,
			BacklotHUDConstants::StatusHeight});
	DrawHealth(*Director);
	DrawTakes(*GameState);
	DrawToolCooldowns(*Director);
	DrawZoneName(*GameState);
	DrawUpload(*GameState, *Director);
	DrawStruggle(*Director);
	DrawTommyGun(*Director);
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
	const bool bIsReady{Director.GetGrappleComponent()
		->GetCooldownFractionRemaining() <= 0.0f};
	const FLinearColor MarkerColor{bIsReady
		? BacklotHUDConstants::WarnColor
		: BacklotHUDConstants::BarBackColor};
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
	if (bIsReady)
	{
		DrawCenteredLabel(TEXT("[E] GRAPPLE"),
			ScreenPosition + FVector2D{0.0f, HalfSize + 14.0f},
			MarkerColor, 1.2f);
	}
}

void ABacklotHUD::DrawGuardGrapplePrompt(
	const ADirectorCharacter& Director)
{
	const UGrappleComponent* Grapple{
		Director.GetGrappleComponent()};
	const AGuardCharacter* Guard{Grapple->GetTargetedGuard()};
	FVector2D ScreenPosition;
	const bool bCanZip{Guard
		&& Grapple->GetCooldownFractionRemaining() <= 0.0f};
	if (!bCanZip || !ProjectToScreen(Guard->GetActorLocation()
		+ FVector{0.0f, 0.0f, 60.0f}, ScreenPosition))
	{
		return;
	}
	DrawCenteredLabel(Guard->IsStunned()
		? TEXT("[E] ZIP IN AND CUT") : TEXT("[E] ZIP TO HIM"),
		ScreenPosition, BacklotHUDConstants::WarnColor, 1.6f);
}

void ABacklotHUD::DrawMegaphoneRings(
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const UMegaphoneComponent* Megaphone{
		Director.GetMegaphoneComponent()};
	const float RingAge{Megaphone->GetSecondsSinceLastNoise()
		/ MegaphoneRingSeconds};
	if (RingAge >= 1.0f)
	{
		return;
	}
	const float FullRadius{
		UBacklotTuningSettings::Get().MegaphoneNoiseRadius};
	const FVector RingCenter{Megaphone->GetLastNoiseLocation()
		- FVector{0.0f, 0.0f, 80.0f}};
	const FLinearColor RingColor{0.4f, 0.8f, 1.0f, 1.0f - RingAge};
	for (const float RingScale : {1.0f, 0.66f, 0.33f})
	{
		DrawGroundRing(RingCenter,
			FullRadius * RingAge * RingScale, RingColor);
	}
}

void ABacklotHUD::DrawGroundRing(const FVector& Center,
	float Radius, const FLinearColor& Color)
{
	using namespace BacklotHUDConstants;
	const float SegmentAngle{2.0f * PI / RingSegmentCount};
	FVector2D PreviousPoint{FVector2D::ZeroVector};
	bool bIsPreviousVisible{false};
	for (int32 PointIndex{0}; PointIndex <= RingSegmentCount;
		++PointIndex)
	{
		const float Angle{SegmentAngle * PointIndex};
		const FVector RingPoint{Center + FVector{
			Radius * FMath::Cos(Angle),
			Radius * FMath::Sin(Angle), 0.0f}};
		FVector2D ScreenPoint;
		const bool bIsVisible{
			ProjectToScreen(RingPoint, ScreenPoint)};
		if (bIsVisible && bIsPreviousVisible)
		{
			DrawLine(PreviousPoint.X, PreviousPoint.Y,
				ScreenPoint.X, ScreenPoint.Y, Color, 3.0f);
		}
		PreviousPoint = ScreenPoint;
		bIsPreviousVisible = bIsVisible;
	}
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
	if (Director.GetTakedownComponent()->IsStruggling())
	{
		return;
	}
	if (Opportunity.bIsInstant)
	{
		DrawCenteredLabel(TEXT("[F] CUT!"), ScreenPosition,
			BacklotHUDConstants::GoodColor, 2.0f);
		return;
	}
	DrawCenteredLabel(TEXT("[F] CUT (slow - he sees you)"),
		ScreenPosition, BacklotHUDConstants::WarnColor, 1.4f);
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
	const FVector2D Position{
		ScreenMargin + PanelInset, ScreenMargin + 9.0f};
	DrawLabel(TEXT("HEALTH"), Position, MutedColor, LabelScale);

	const int32 MaximumHealth{FMath::Max(1, Health->GetMaximumHealth())};
	const float PipPitch{118.0f / MaximumHealth};
	const float PipWidth{PipPitch * 0.82f};
	for (int32 PipIndex{0}; PipIndex < MaximumHealth; ++PipIndex)
	{
		const bool bIsFilled{PipIndex < Health->GetCurrentHealth()};
		const float PipX{static_cast<float>(Position.X)
			+ PipIndex * PipPitch};
		const float PipY{static_cast<float>(Position.Y) + 26.0f};
		DrawRect(RuleColor, PipX, PipY, PipWidth, 9.0f);
		DrawRect(bIsFilled ? BadColor : BarBackColor,
			PipX + PipWidth * 0.08f, PipY + 1.0f,
			PipWidth * 0.84f, 7.0f);
	}
	DrawRect(RuleColor, Position.X, ScreenMargin + 49.0f,
		StatusWidth - PanelInset * 2.0f, 1.0f);
}

void ABacklotHUD::DrawTakes(const ABacklotGameState& GameState)
{
	using namespace BacklotHUDConstants;
	const int32 TakesRemaining{GameState.GetTakesRemaining()};
	const FVector2D Position{ScreenMargin + 172.0f, ScreenMargin};
	DrawCenteredLabel(TEXT("TAKES"), Position + FVector2D{0.0f, 17.0f},
		MutedColor, LabelScale);
	DrawCenteredLabel(FString::FromInt(TakesRemaining),
		Position + FVector2D{0.0f, 34.0f},
		TakesRemaining > 1 ? TextColor : BadColor, 1.25f);
	DrawRect(RuleColor, ScreenMargin + 139.0f, ScreenMargin + 11.0f,
		1.0f, 29.0f);
}

void ABacklotHUD::DrawToolCooldowns(
	const ADirectorCharacter& Director)
{
	DrawToolCooldown(TEXT("FLASH"), TEXT("Q"),
		Director.GetCameraFlashComponent()
			->GetCooldownFractionRemaining(), 0);
	DrawToolCooldown(TEXT("MEGAPHONE"), TEXT("G"),
		Director.GetMegaphoneComponent()
			->GetCooldownFractionRemaining(), 1);
	DrawToolCooldown(TEXT("GRAPPLE"), TEXT("E"),
		Director.GetGrappleComponent()
			->GetCooldownFractionRemaining(), 2);
}

void ABacklotHUD::DrawToolCooldown(const FString& ToolName,
	const FString& Key, float CooldownFractionRemaining, int32 RowIndex)
{
	using namespace BacklotHUDConstants;
	const FVector2D Position{ScreenMargin + PanelInset,
		ScreenMargin + 59.0f + RowIndex * 37.0f};
	const bool bIsReady{CooldownFractionRemaining <= 0.0f};
	DrawRect(RuleColor, Position.X, Position.Y, 22.0f, 22.0f);
	DrawRect(BarBackColor, Position.X + 1.0f, Position.Y + 1.0f,
		20.0f, 20.0f);
	DrawCenteredLabel(Key, Position + FVector2D{11.0f, 11.0f},
		WarnColor, 0.95f);
	DrawLabel(ToolName, Position + FVector2D{31.0f, 1.0f},
		TextColor, 0.96f);
	const FString Status{bIsReady ? TEXT("READY") : TEXT("RECHARGING")};
	float StatusWidthPixels{};
	float StatusHeightPixels{};
	GetTextSize(Status, StatusWidthPixels, StatusHeightPixels,
		GEngine->GetMediumFont(), LabelScale);
	DrawLabel(Status, Position + FVector2D{
		StatusWidth - PanelInset * 2.0f - StatusWidthPixels, 2.0f},
		bIsReady ? GoodColor : WarnColor, LabelScale);
	DrawBar(Position + FVector2D{31.0f, 23.0f},
		FVector2D{StatusWidth - PanelInset * 2.0f - 31.0f, 2.0f},
		1.0f - CooldownFractionRemaining, bIsReady ? GoodColor : WarnColor);
}

void ABacklotHUD::DrawStruggle(const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const UTakedownComponent* Takedown{
		Director.GetTakedownComponent()};
	if (!Takedown->IsStruggling())
	{
		return;
	}
	const FVector2D BarSize{300.0f, 4.0f};
	const FVector2D BarPosition{
		Canvas->SizeX * 0.5f - BarSize.X * 0.5f,
		Canvas->SizeY * 0.6f};
	DrawDecoPanel(BarPosition - FVector2D{15.0f, 32.0f},
		BarSize + FVector2D{30.0f, 44.0f});
	DrawBar(BarPosition, BarSize, Takedown->GetStruggleProgress(),
		WarnColor);
	DrawCenteredLabel(TEXT("CUTTING...  you cannot move"),
		BarPosition + FVector2D{BarSize.X * 0.5f, -16.0f},
		WarnColor, 1.4f);
}

void ABacklotHUD::DrawTommyGun(const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const UTommyGunComponent* TommyGun{
		Director.GetTommyGunComponent()};
	if (!TommyGun->IsEquipped())
	{
		return;
	}
	DrawLabel(TEXT("TOMMY GUN [Left mouse]"), FVector2D{
		ScreenMargin, ScreenMargin + StatusHeight + 10.0f}, TextColor, 1.0f);
	if (TommyGun->GetSecondsSinceLastHit() > HitMarkerSeconds)
	{
		return;
	}
	const FVector2D ScreenCenter{
		Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f};
	const float InnerOffset{6.0f};
	const float OuterOffset{16.0f};
	for (const FVector2D& Corner : {FVector2D{1.0f, 1.0f},
		FVector2D{1.0f, -1.0f}, FVector2D{-1.0f, 1.0f},
		FVector2D{-1.0f, -1.0f}})
	{
		const FVector2D LineStart{
			ScreenCenter + Corner * InnerOffset};
		const FVector2D LineEnd{
			ScreenCenter + Corner * OuterOffset};
		DrawLine(LineStart.X, LineStart.Y, LineEnd.X, LineEnd.Y,
			BadColor, 3.0f);
	}
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
	using namespace BacklotHUDConstants;
	const FString ZoneName{GameState.GetCurrentZoneName().ToUpper()};
	if (ZoneName.IsEmpty())
	{
		return;
	}
	float Width{};
	float Height{};
	GetTextSize(ZoneName, Width, Height, GEngine->GetMediumFont(), 1.45f);
	const float AvailableWidth{FMath::Max(80.0f,
		Canvas->SizeX - 2.0f * (ScreenMargin + StatusWidth + 12.0f))};
	const float Scale{1.45f * FMath::Min(1.0f,
		(AvailableWidth - 32.0f) / FMath::Max(Width, 1.0f))};
	const FVector2D Size{
		FMath::Min(AvailableWidth, FMath::Max(Width + 40.0f, 166.0f)),
		38.0f};
	const FVector2D Position{Canvas->SizeX * 0.5f - Size.X * 0.5f,
		ScreenMargin};
	DrawDecoPanel(Position, Size);
	DrawCenteredLabel(ZoneName, Position + Size * 0.5f, TextColor, Scale);
}

void ABacklotHUD::DrawUpload(const ABacklotGameState& GameState,
	const ADirectorCharacter& Director)
{
	using namespace BacklotHUDConstants;
	const FVector2D BarSize{
		FMath::Min(440.0f, Canvas->SizeX - 2.0f * ScreenMargin - 30.0f),
		4.0f};
	const FVector2D BarPosition{
		Canvas->SizeX * 0.5f - BarSize.X * 0.5f,
		FMath::Max(ScreenMargin + 82.0f,
			Canvas->SizeX < 1024 ? ScreenMargin + StatusHeight + 42.0f
				: 0.0f)};
	if (GameState.IsUploadActive())
	{
		const bool bIsPaused{GameState.IsUploadPaused()};
		const float Progress{GameState.GetUploadProgress()};
		DrawDecoPanel(BarPosition - FVector2D{15.0f, 32.0f},
			BarSize + FVector2D{30.0f, 44.0f});
		DrawBar(BarPosition, BarSize, Progress,
			bIsPaused ? BadColor : GoodColor);
		DrawCenteredLabel(bIsPaused
			? TEXT("UPLOAD PAUSED - GET BACK TO THE TERMINAL")
			: FString::Printf(TEXT("UPLOADING THE FILM  %d%%"),
				FMath::FloorToInt32(Progress * 100.0f)),
			BarPosition + FVector2D{BarSize.X * 0.5f, -16.0f},
			bIsPaused ? BadColor : TextColor, 1.05f);
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


void ABacklotHUD::DrawControlsHelp()
{
	using namespace BacklotHUDConstants;
	float LongestLineWidth{0.0f};
	for (const FString& Line : ControlsLines)
	{
		float Width{};
		float Height{};
		GetTextSize(Line, Width, Height, GEngine->GetMediumFont(), 1.0f);
		LongestLineWidth = FMath::Max(LongestLineWidth, Width);
	}

	const float PanelWidth{FMath::Min(LongestLineWidth + PanelInset * 2.0f,
		Canvas->SizeX * 0.42f)};
	const float TextScale{FMath::Min(1.0f,
		(PanelWidth - PanelInset * 2.0f) / LongestLineWidth)};
	const float LineHeight{20.0f * TextScale};
	const FVector2D Position{Canvas->SizeX - PanelWidth - ScreenMargin,
		ScreenMargin + 48.0f};
	DrawDecoPanel(Position, FVector2D{PanelWidth,
		ControlsLines.Num() * LineHeight + 31.0f});
	DrawLabel(ControlsLines[0], Position + FVector2D{PanelInset, 8.0f},
		WarnColor, 1.15f);
	DrawRect(RuleColor, Position.X + PanelInset, Position.Y + 31.0f,
		PanelWidth - PanelInset * 2.0f, 1.0f);
	for (int32 LineIndex{1}; LineIndex < ControlsLines.Num(); ++LineIndex)
	{
		DrawLabel(ControlsLines[LineIndex], Position
			+ FVector2D{PanelInset, 36.0f + (LineIndex - 1) * LineHeight},
			TextColor, TextScale);
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

void ABacklotHUD::DrawWinScreen(const ABacklotGameState& GameState)
{
	using namespace BacklotHUDConstants;
	const int32 TakesRemaining{GameState.GetTakesRemaining()};
	DrawEndScreen(TEXT("THAT'S A WRAP!"),
		TEXT("The director's cut is uploaded. You win."),
		FString::Printf(TEXT("Wrapped with %d %s to spare"),
			TakesRemaining,
			TakesRemaining == 1 ? TEXT("take") : TEXT("takes")),
		WarnColor);
}

void ABacklotHUD::DrawLoseScreen()
{
	using namespace BacklotHUDConstants;
	DrawEndScreen(TEXT("GAME OVER"),
		TEXT("CUT! The studio pulled the plug on your picture."),
		TEXT("You ran out of takes"), BadColor);
}

void ABacklotHUD::DrawEndScreen(const FString& Title,
	const FString& Subtitle, const FString& Detail,
	const FLinearColor& TitleColor)
{
	using namespace BacklotHUDConstants;
	DrawRect(FLinearColor{0.0f, 0.0f, 0.0f, 0.75f}, 0.0f, 0.0f,
		Canvas->SizeX, Canvas->SizeY);
	const FVector2D ScreenCenter{
		Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f};
	const FVector2D CardSize{
		FMath::Min(640.0f, Canvas->SizeX - 2.0f * ScreenMargin), 244.0f};
	DrawDecoPanel(ScreenCenter - CardSize * 0.5f, CardSize);
	DrawCenteredLabel(Title,
		ScreenCenter - FVector2D{0.0f, 64.0f}, TitleColor, 3.3f);
	DrawRect(RuleColor, ScreenCenter.X - CardSize.X * 0.38f,
		ScreenCenter.Y - 27.0f, CardSize.X * 0.76f, 1.0f);
	DrawCenteredLabel(Subtitle,
		ScreenCenter + FVector2D{0.0f, -1.0f}, TextColor, 1.35f);
	DrawCenteredLabel(Detail,
		ScreenCenter + FVector2D{0.0f, 24.0f}, MutedColor, 1.05f);
	const FVector2D RestartSize{156.0f, 34.0f};
	const FVector2D RestartCenter{ScreenCenter + FVector2D{0.0f, 79.0f}};
	DrawDecoPanel(RestartCenter - RestartSize * 0.5f, RestartSize);
	DrawCenteredLabel(TEXT("[R] RESTART"), RestartCenter, WarnColor, 1.0f);
}

void ABacklotHUD::DrawCrosshair()
{
	const float DotSize{4.0f};
	DrawRect(FLinearColor{1.0f, 1.0f, 1.0f, 0.7f},
		Canvas->SizeX * 0.5f - DotSize * 0.5f,
		Canvas->SizeY * 0.5f - DotSize * 0.5f, DotSize, DotSize);
}

void ABacklotHUD::DrawDecoPanel(const FVector2D& Position,
	const FVector2D& Size)
{
	using namespace BacklotHUDConstants;
	constexpr float Corner{7.0f};
	DrawRect(PanelColor, Position.X + Corner, Position.Y,
		Size.X - Corner * 2.0f, Size.Y);
	DrawRect(PanelColor, Position.X, Position.Y + Corner, Corner,
		Size.Y - Corner * 2.0f);
	DrawRect(PanelColor, Position.X + Size.X - Corner, Position.Y + Corner,
		Corner, Size.Y - Corner * 2.0f);
	for (float Y : {Position.Y, Position.Y + Size.Y - 1.0f})
	{
		DrawRect(RuleColor, Position.X + Corner, Y,
			Size.X - 2.0f * Corner, 1.0f);
		DrawRect(WarnColor, Position.X + Size.X * 0.5f - 17.0f,
			Y, 34.0f, 1.0f);
	}
	for (float X : {Position.X, Position.X + Size.X - 1.0f})
	{
		DrawRect(RuleColor, X, Position.Y + Corner, 1.0f,
			Size.Y - 2.0f * Corner);
	}
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
