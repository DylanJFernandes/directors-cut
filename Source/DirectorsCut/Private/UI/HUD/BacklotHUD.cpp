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

	/* Seconds a megaphone ring takes to reach its full size,
	   and the number of straight pieces it is drawn with */
	constexpr float MegaphoneRingSeconds{0.9f};
	constexpr int32 RingSegmentCount{40};

	/* Seconds the tommy gun hit marker stays up */
	constexpr float HitMarkerSeconds{0.08f};

	const FLinearColor BarBackColor{0.0f, 0.0f, 0.0f, 0.6f};
	const FLinearColor TextColor{1.0f, 1.0f, 1.0f};
	const FLinearColor GoodColor{0.3f, 1.0f, 0.4f};
	const FLinearColor WarnColor{1.0f, 0.85f, 0.1f};
	const FLinearColor BadColor{1.0f, 0.2f, 0.2f};

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
		DrawWinScreen();
		return;
	}
	DrawGuardMeters();
	DrawGrappleMarker(*Director);
	DrawGuardGrapplePrompt(*Director);
	DrawMegaphoneRings(*Director);
	DrawTakedownPrompt(*Director);
	DrawWorldPopups();
	DrawScreenFlashes(*Director);
	DrawHealth(*Director);
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

void ABacklotHUD::DrawToolCooldowns(
	const ADirectorCharacter& Director)
{
	DrawToolCooldown(TEXT("FLASH [Q]"),
		Director.GetCameraFlashComponent()
			->GetCooldownFractionRemaining(), 0);
	DrawToolCooldown(TEXT("MEGAPHONE [G]"),
		Director.GetMegaphoneComponent()
			->GetCooldownFractionRemaining(), 1);
	DrawToolCooldown(TEXT("GRAPPLE [E]"),
		Director.GetGrappleComponent()
			->GetCooldownFractionRemaining(), 2);
}

void ABacklotHUD::DrawToolCooldown(const FString& ToolName,
	float CooldownFractionRemaining, int32 RowIndex)
{
	using namespace BacklotHUDConstants;
	const float RowHeight{46.0f};
	const bool bIsReady{CooldownFractionRemaining <= 0.0f};
	const FVector2D LabelPosition{ScreenMargin,
		ScreenMargin + 58.0f + RowIndex * RowHeight};
	DrawLabel(ToolName + (bIsReady
		? TEXT("  READY") : TEXT("  recharging")),
		LabelPosition, bIsReady ? GoodColor : TextColor, 1.2f);
	DrawBar(LabelPosition + FVector2D{0.0f, 24.0f},
		FVector2D{154.0f, 12.0f}, 1.0f - CooldownFractionRemaining,
		bIsReady ? GoodColor : WarnColor);
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
	const FVector2D BarSize{320.0f, 18.0f};
	const FVector2D BarPosition{
		Canvas->SizeX * 0.5f - BarSize.X * 0.5f,
		Canvas->SizeY * 0.6f};
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
		ScreenMargin, ScreenMargin + 200.0f}, GoodColor, 1.2f);
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
