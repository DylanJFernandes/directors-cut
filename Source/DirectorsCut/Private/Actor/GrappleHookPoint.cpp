#include "Actor/GrappleHookPoint.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/GreyboxShapeLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

AGrappleHookPoint::AGrappleHookPoint()
	: IdleColor{0.1f, 0.8f, 1.0f},
	  TargetedColor{1.0f, 0.85f, 0.1f},
	  IdleSphereScale{0.45f},
	  TargetedSphereScale{0.75f}
{
	PrimaryActorTick.bCanEverTick = false;

	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("SphereMesh"));
	SetRootComponent(SphereMesh);
	SphereMesh->SetMobility(EComponentMobility::Movable);
	SphereMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	SphereMesh->SetCanEverAffectNavigation(false);
	SphereMesh->SetCastShadow(false);

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(
		TEXT("GlowLight"));
	GlowLight->SetupAttachment(SphereMesh);
	GlowLight->SetIntensity(500.0f);
	GlowLight->SetAttenuationRadius(260.0f);
	GlowLight->SetCastShadows(false);
}

void AGrappleHookPoint::BeginPlay()
{
	Super::BeginPlay();
	SphereMesh->SetStaticMesh(
		GreyboxShapeLibrary::GetSphereMesh());
	SphereMaterial = GreyboxShapeLibrary::ApplyGlowColor(
		SphereMesh, IdleColor);
	ShowAsIdle();
}

void AGrappleHookPoint::ShowAsTargeted()
{
	ApplyLook(TargetedColor, TargetedSphereScale);
}

void AGrappleHookPoint::ShowAsIdle()
{
	ApplyLook(IdleColor, IdleSphereScale);
}

void AGrappleHookPoint::ApplyLook(const FLinearColor& Color,
	float SphereScale)
{
	SphereMesh->SetWorldScale3D(FVector{SphereScale});
	GlowLight->SetLightColor(Color);
	if (SphereMaterial)
	{
		SphereMaterial->SetVectorParameterValue(TEXT("Color"),
			Color);
	}
}
