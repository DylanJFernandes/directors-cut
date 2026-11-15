#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "BeamMeshComponent.generated.h"

class UMaterialInstanceDynamic;

/* A thin stretched box drawn between two world points. Used for
   the guard laser telegraph and the film reel tether. */
UCLASS()
class UBeamMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	/* Sets the beam up as a hidden, collision-free box */
	UBeamMeshComponent();

	/* Shows the beam stretched from one point to the other */
	void ShowBeamBetween(const FVector& StartLocation,
		const FVector& EndLocation);

	/* Hides the beam */
	void HideBeam();

	/* Sets how thick the beam is drawn, in units */
	void SetBeamThickness(float InBeamThickness);

	/* Sets the beam color */
	void SetBeamColor(const FLinearColor& BeamColor);

protected:
	/* Loads the cube mesh and creates the beam material */
	virtual void BeginPlay() override;

private:
	/* Thickness of the beam, in units */
	float BeamThickness;

	/* Material instance that carries the beam color */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMaterial;
};
