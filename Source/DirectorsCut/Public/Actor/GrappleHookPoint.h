#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleHookPoint.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/* A point the film reel grapple can latch onto. Shown as a
   glowing sphere that changes color while it is targeted. */
UCLASS()
class AGrappleHookPoint : public AActor
{
	GENERATED_BODY()

public:
	/* Creates the sphere and its light */
	AGrappleHookPoint();

	/* Marks this as the hook point E will grapple to */
	void ShowAsTargeted();

	/* Returns this hook point to its idle look */
	void ShowAsIdle();

protected:
	/* Applies the idle look */
	virtual void BeginPlay() override;

private:
	/* Applies a color and size to the sphere and light */
	void ApplyLook(const FLinearColor& Color, float SphereScale);

	/* Sphere that marks the hook point */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> SphereMesh;

	/* Light that makes the hook point glow */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UPointLightComponent> GlowLight;

	/* Color while no grapple is aimed at this point */
	UPROPERTY(EditAnywhere, Category = "Custom|Look")
	FLinearColor IdleColor;

	/* Color while this point is the grapple target */
	UPROPERTY(EditAnywhere, Category = "Custom|Look")
	FLinearColor TargetedColor;

	/* Sphere scale while idle */
	UPROPERTY(EditAnywhere, Category = "Custom|Look")
	float IdleSphereScale;

	/* Sphere scale while targeted */
	UPROPERTY(EditAnywhere, Category = "Custom|Look")
	float TargetedSphereScale;

	/* Material instance that carries the sphere color */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SphereMaterial;
};
