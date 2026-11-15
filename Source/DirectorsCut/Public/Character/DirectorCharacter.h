#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DirectorCharacter.generated.h"

class AGuardCharacter;
class UBeamMeshComponent;
class UCameraComponent;
class UCameraFlashComponent;
class UDirectorHealthComponent;
class UDirectorLocomotionComponent;
class UGrappleComponent;
class UInputAction;
class UInputMappingContext;
class UMegaphoneComponent;
class UPointLightComponent;
class USpringArmComponent;
class UTakedownComponent;
class UTommyGunComponent;
struct FInputActionValue;

/* The film director the player controls. Walks, sprints,
   slides, jumps and crouches, and carries his tools as
   components: the film reel grapple, the camera flash, the
   clapperboard takedown, the megaphone and the tommy gun. */
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

	/* Returns the sprint and slide component */
	UDirectorLocomotionComponent* GetLocomotionComponent() const;

	/* Returns the tommy gun component */
	UTommyGunComponent* GetTommyGunComponent() const;

	/* Hands the director the tommy gun */
	void EquipTommyGun();

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

	/* Starts a slide that was queued in the air */
	virtual void Landed(const FHitResult& Hit) override;

	/* Tells the slide when the director runs off a ledge */
	virtual void OnMovementModeChanged(
		EMovementMode PreviousMovementMode,
		uint8 PreviousCustomMode) override;

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

	/* Starts a slide when fast enough, else toggles crouching */
	void HandleCrouchPressed();

	/* Starts sprinting */
	void HandleSprintPressed();

	/* Stops sprinting */
	void HandleSprintReleased();

	/* Starts the upload when next to the terminal, otherwise
	   launches the film reel grapple at the targeted guard or
	   hook point */
	void HandleInteractPressed();

	/* Shouts through the megaphone to draw guards over */
	void HandleMegaphonePressed();

	/* Starts firing the tommy gun */
	void HandleFirePressed();

	/* Stops firing the tommy gun */
	void HandleFireReleased();

	/* Knocks out a stunned guard the grapple just reached */
	void HandleGrappleReachedGuard(AGuardCharacter& Guard);

	/* Returns whether the director is held in place by a slow
	   takedown and cannot use his tools */
	bool IsBusy() const;

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

	/* Sprinting and sliding */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UDirectorLocomotionComponent> Locomotion;

	/* Film reel tommy gun, hidden until it is picked up */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UTommyGunComponent> TommyGun;

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

	/* Crouch toggle or slide on C or Left Ctrl */
	UPROPERTY()
	TObjectPtr<UInputAction> CrouchAction;

	/* Sprint while Left Shift is held */
	UPROPERTY()
	TObjectPtr<UInputAction> SprintAction;

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

	/* Tommy gun on the left mouse button */
	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;

	/* Timer that switches the flash light off */
	FTimerHandle FlashLightTimerHandle;
};
