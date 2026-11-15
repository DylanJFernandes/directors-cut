#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TommyGunPickup.generated.h"

class UTextRenderComponent;

/* The film reel tommy gun, floating and turning in the air
   until the director walks into it and takes it */
UCLASS()
class ATommyGunPickup : public AActor
{
	GENERATED_BODY()

public:
	/* Creates the root the gun's pieces attach to */
	ATommyGunPickup();

protected:
	/* Builds the gun's shape */
	virtual void BeginPlay() override;

	/* Turns and bobs the gun and hands it to the director
	   when he is close enough */
	virtual void Tick(float DeltaSeconds) override;

private:
	/* Part that turns and bobs, carrying the gun's pieces */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<USceneComponent> GunRoot;

	/* Name floating above the gun */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UTextRenderComponent> Label;

	/* Degrees per second the gun turns */
	UPROPERTY(EditAnywhere, Category = "Custom|Look")
	float TurnRateDegreesPerSecond;

	/* How far the gun bobs up and down */
	UPROPERTY(EditAnywhere, Category = "Custom|Look")
	float BobHeight;
};