#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DirectorCharacter.generated.h"

class ACameraRigTurret;
class UBeamMeshComponent;
class UCameraComponent;
class UCameraFlashComponent;
class UDirectorHealthComponent;
class UGrappleComponent;
class UInputAction;
class UInputMappingContext;
class UMegaphoneComponent;
class UPointLightComponent;
class USpringArmComponent;
class UTakedownComponent;
struct FInputActionValue;

/* The film director the player controls. Runs, jumps and
   crouches, and carries his tools as components: the film reel
   grapple, the camera flash and the clapperboard takedown. */
UCLASS()
class ADirectorCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/* Creates the camera, mesh and tool components */
	ADirectorCharacter();

	/* Takes one hit from a guard's shot */
	void ReceiveGuardShot(const FVector& ShotOrigin);

	/* Returns the director to a checkpoint at full health */
	void RespawnAt(const FTransform& RespawnTransform);

	/* Returns the health component */
	UDirectorHealthComponent* GetHealthComponent() const;

	/* Returns the film reel grapple component */
	UGrappleComponent* GetGrappleComponent() const;

	/* Returns the camera flash component */
	UCameraFlashComponent* GetCameraFlashComponent() const;

	/* Returns the clapperboard takedown component */
	UTakedownComponent* GetTakedownComponent() const;

	/* Returns the megaphone component */
	UMegaphoneComponent* GetMegaphoneComponent() const;

	/* Returns whether the director is on a camera rig */
	bool IsMountedOnCameraRig() const;

protected:
	/* Applies tuning values and binds the health delegate */
	virtual void BeginPlay() override;

	/* Draws the film reel tether while grappling */
	virtual void Tick(float DeltaSeconds) override;

	/* Creates the input actions in code and binds them */
	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent) override;

	/* Squashes the mesh so crouching reads without a crouch
	   animation */
	virtual void OnStartCrouch(float HalfHeightAdjust,
		float ScaledHalfHeightAdjust) override;

	/* Restores the mesh height after crouching */
	virtual void OnEndCrouch(float HalfHeightAdjust,
		float ScaledHalfHeightAdjust) override;

	/* Treats falling out of the level as being caught */
	virtual void FellOutOfWorld(
		const UDamageType& DamageType) override;

private:
	/* Copies movement and camera tuning onto the character */
	void ApplyTuning();

	/* Creates every input action and the mapping context */
	void CreateInputActions();

	/* Creates one button-style input action bound to a key */
	UInputAction* CreateButtonAction(const FKey& Key);

	/* Maps a key to one direction of the move action */
	void MapMoveKey(const FKey& Key, bool bSwapAxes,
		bool bNegate);

	/* Registers the mapping context with the local player */
	void AddMappingContext() const;

	/* Binds every input action to its handler */
	void BindInputActions(UInputComponent* PlayerInputComponent);

	/* Moves relative to the camera */
	void HandleMove(const FInputActionValue& ActionValue);

	/* Turns the camera */
	void HandleLook(const FInputActionValue& ActionValue);

	/* Jumps, standing up first if crouched */
	void HandleJumpPressed();

	/* Stops a held jump */
	void HandleJumpReleased();

	/* Toggles crouching */
	void HandleCrouchPressed();

	/* Starts the upload when next to the terminal, reels in a
	   stunned guard when aiming at one, otherwise launches the
	   film reel grapple */
	void HandleInteractPressed();

	/* Throws a noise to the aimed spot with the megaphone */
	void HandleMegaphonePressed();

	/* Fires the camera rig's spotlight while mounted */
	void HandleFirePressed();

	/* Reels in the stunned guard being aimed at. Returns
	   whether a guard was reeled in. */
	bool TryReelInStunnedGuard();

	/* Climbs onto a camera rig in reach. Returns whether the
	   director mounted one. */
	bool TryMountCameraRig();

	/* Climbs off the camera rig */
	void DismountCameraRig();

	/* Climbs off if something knocked the director away */
	void DismountIfKnockedOffCameraRig();

	/* Fires the camera flash */
	void HandleFlashPressed();

	/* Attempts a clapperboard takedown */
	void HandleTakedownPressed();

	/* Starts the upload if a terminal is in reach. Returns
	   whether a terminal was used. */
	bool TryUseUploadTerminal();

	/* Shows or hides the film reel tether */
	void UpdateGrappleTether();

	/* Tells the game mode the director was caught */
	void HandleHealthDepleted();

	/* Switches the bright flash light off again */
	void HideFlashLight();

	/* Arm that keeps the camera behind the director */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/* Third person camera */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	/* Bright light shown for an instant when the flash fires */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UPointLightComponent> FlashLight;

	/* Film strip drawn to the hook point while grappling */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UBeamMeshComponent> GrappleTether;

	/* Health and regeneration */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UDirectorHealthComponent> Health;

	/* Film reel grapple */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UGrappleComponent> Grapple;

	/* Camera flash */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UCameraFlashComponent> CameraFlash;

	/* Clapperboard takedown */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UTakedownComponent> Takedown;

	/* Megaphone */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UMegaphoneComponent> Megaphone;

	// Input
	/* Mapping context holding every key binding */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DirectorMappingContext;

	/* WASD movement */
	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	/* Mouse look */
	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	/* Jump on Space */
	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;

	/* Crouch toggle on C or Left Ctrl */
	UPROPERTY()
	TObjectPtr<UInputAction> CrouchAction;

	/* Grapple or use the terminal on E */
	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;

	/* Camera flash on Q */
	UPROPERTY()
	TObjectPtr<UInputAction> FlashAction;

	/* Clapperboard takedown on F */
	UPROPERTY()
	TObjectPtr<UInputAction> TakedownAction;

	/* Megaphone on G */
	UPROPERTY()
	TObjectPtr<UInputAction> MegaphoneAction;

	/* Camera rig spotlight on the left mouse button */
	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;

	/* Camera rig the director is standing on, if any */
	TWeakObjectPtr<ACameraRigTurret> MountedCameraRig;

	/* Timer that switches the flash light off */
	FTimerHandle FlashLightTimerHandle;
};
