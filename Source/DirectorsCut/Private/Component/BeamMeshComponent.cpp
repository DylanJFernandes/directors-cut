#include "Component/BeamMeshComponent.h"

#include "Game/GreyboxShapeLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

UBeamMeshComponent::UBeamMeshComponent()
	: BeamThickness{4.0f}
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);
	SetHiddenInGame(true);
}

void UBeamMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	SetStaticMesh(GreyboxShapeLibrary::GetCubeMesh());
	BeamMaterial = GreyboxShapeLibrary::ApplyGlowColor(this,
		FLinearColor::Red);
}

void UBeamMeshComponent::ShowBeamBetween(
	const FVector& StartLocation, const FVector& EndLocation)
{
	const FVector BeamVector{EndLocation - StartLocation};
	const float BeamLength{
		static_cast<float>(BeamVector.Size())};
	const float ThicknessScale{
		BeamThickness / GreyboxShapeLibrary::BasicShapeSize};

	SetWorldLocation((StartLocation + EndLocation) * 0.5f);
	SetWorldRotation(BeamVector.Rotation());
	SetWorldScale3D(FVector{
		BeamLength / GreyboxShapeLibrary::BasicShapeSize,
		ThicknessScale, ThicknessScale});
	SetHiddenInGame(false);
}

void UBeamMeshComponent::HideBeam()
{
	SetHiddenInGame(true);
}

void UBeamMeshComponent::SetBeamThickness(float InBeamThickness)
{
	BeamThickness = InBeamThickness;
}

void UBeamMeshComponent::SetBeamColor(
	const FLinearColor& BeamColor)
{
	if (BeamMaterial)
	{
		BeamMaterial->SetVectorParameterValue(TEXT("Color"),
			BeamColor);
	}
}
