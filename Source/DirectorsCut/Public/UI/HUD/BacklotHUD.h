#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BacklotHUD.generated.h"

class ABacklotGameState;
class ADirectorCharacter;
class AGuardCharacter;

/* A short text that floats up from a place in the world */
struct FWorldPopup
{
	/* Text to show */
	FString Text;

	/* World location the text floats above */
	FVector WorldLocation{FVector::ZeroVector};

	/* Color of the text */
	FLinearColor Color{FLinearColor::White};

	/* World time at which the popup appeared */
	float StartTimeSeconds{0.0f};
};

/* The whole HUD, drawn on the canvas: health, flash cooldown,
   zone name, upload bar, guard detection meters, the grapple
   target marker, prompts, popups and the win screen. */
UCLASS()
class ABacklotHUD : public AHUD
{
	GENERATED_BODY()

public:
	/* Sets up an empty HUD */
	ABacklotHUD();

	/* Draws every HUD element for this frame */
	virtual void DrawHUD() override;

	/* Shows a short text floating above a world location */
	void ShowWorldPopup(const FString& Text,
		const FVector& WorldLocation, const FLinearColor& Color);

	/* Shows a message in the middle of the screen for a while */
	void ShowCenterMessage(const FString& Message);

private:
	// World-anchored elements
	/* Draws the detection meter above every guard in view */
	void DrawGuardMeters();

	/* Draws one guard's meter and state label */
	void DrawGuardMeter(const AGuardCharacter& Guard);

	/* Marks the hook point E would grapple to */
	void DrawGrappleMarker(const ADirectorCharacter& Director);

	/* Marks the stunned guard E would reel in */
	void DrawReelInPrompt(const ADirectorCharacter& Director);

	/* Shows the takedown prompt on the guard in reach */
	void DrawTakedownPrompt(const ADirectorCharacter& Director);

	/* Draws and expires the floating popups */
	void DrawWorldPopups();

	// Screen elements
	/* Draws the health pips */
	void DrawHealth(const ADirectorCharacter& Director);

	/* Draws the camera flash cooldown bar */
	void DrawFlashCooldown(const ADirectorCharacter& Director);

	/* Draws the megaphone cooldown bar */
	void DrawMegaphoneCooldown(const ADirectorCharacter& Director);

	/* Whites the screen out briefly after a flash and tints it
	   red after a hit */
	void DrawScreenFlashes(const ADirectorCharacter& Director);

	/* Draws the current zone name */
	void DrawZoneName(const ABacklotGameState& GameState);

	/* Draws the upload bar, or the prompt to start it */
	void DrawUpload(const ABacklotGameState& GameState,
		const ADirectorCharacter& Director);

	/* Shows how to mount or use the camera rig */
	void DrawCameraRigPrompt(const ADirectorCharacter& Director);

	/* Lists the controls */
	void DrawControlsHelp();

	/* Draws the timed message in the middle of the screen */
	void DrawCenterMessage();

	/* Draws the win screen */
	void DrawWinScreen();

	/* Draws the dot in the middle of the screen */
	void DrawCrosshair();

	// Drawing helpers
	/* Draws text with its top left corner at a position */
	void DrawLabel(const FString& Text, const FVector2D& Position,
		const FLinearColor& Color, float Scale);

	/* Draws text centered on a position */
	void DrawCenteredLabel(const FString& Text,
		const FVector2D& Center, const FLinearColor& Color,
		float Scale);

	/* Draws a bar with a filled fraction */
	void DrawBar(const FVector2D& Position, const FVector2D& Size,
		float FilledFraction, const FLinearColor& FillColor);

	/* Converts a world location to a screen position. Returns
	   false when the location is behind the camera. */
	bool ProjectToScreen(const FVector& WorldLocation,
		FVector2D& OutScreenPosition) const;

	/* Returns the director character */
	ADirectorCharacter* GetDirector() const;

	/* Popups currently floating */
	TArray<FWorldPopup> WorldPopups;

	/* Message shown in the middle of the screen */
	FString CenterMessage;

	/* World time at which the center message appeared */
	float CenterMessageStartTimeSeconds;
};
