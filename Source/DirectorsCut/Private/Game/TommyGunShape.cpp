#include "Game/TommyGunShape.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/GreyboxShapeLibrary.h"

namespace TommyGunShape
{
	const FLinearColor GunMetal{0.04f, 0.04f, 0.05f};
	const FLinearColor GunWood{0.3f, 0.17f, 0.08f};
	const FLinearColor FilmReel{0.75f, 0.75f, 0.8f};

	/* Turns a cylinder so its flat faces point sideways */
	const FRotator ReelRotation{0.0f, 0.0f, 90.0f};

	/* Adds one collision-free piece of the gun */
	void AddPiece(USceneComponent& Parent, UStaticMesh* Mesh,
		const FTransform& RelativeTransform,
		const FLinearColor& Color)
	{
		UStaticMeshComponent* Piece{
			NewObject<UStaticMeshComponent>(Parent.GetOwner())};
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetStaticMesh(Mesh);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCanEverAffectNavigation(false);
		Piece->SetCastShadow(false);
		Piece->SetupAttachment(&Parent);
		Piece->SetRelativeTransform(RelativeTransform);
		Piece->RegisterComponent();
		GreyboxShapeLibrary::ApplyFlatColor(Piece, Color);
	}
}

void TommyGunShape::Build(USceneComponent& Parent)
{
	UStaticMesh* Cube{GreyboxShapeLibrary::GetCubeMesh()};
	UStaticMesh* Cylinder{GreyboxShapeLibrary::GetCylinderMesh()};
	AddPiece(Parent, Cube, FTransform{FRotator::ZeroRotator,
		FVector{0.0f, 0.0f, 0.0f}, FVector{0.5f, 0.07f, 0.1f}},
		GunMetal);
	AddPiece(Parent, Cube, FTransform{FRotator::ZeroRotator,
		FVector{-34.0f, 0.0f, -4.0f}, FVector{0.2f, 0.06f, 0.12f}},
		GunWood);
	AddPiece(Parent, Cube, FTransform{FRotator::ZeroRotator,
		FVector{-10.0f, 0.0f, -10.0f},
		FVector{0.06f, 0.05f, 0.12f}}, GunWood);
	AddPiece(Parent, Cylinder, FTransform{ReelRotation,
		FVector{34.0f, 0.0f, 0.0f}, FVector{0.3f, 0.3f, 0.07f}},
		FilmReel);
	AddPiece(Parent, Cylinder, FTransform{ReelRotation,
		FVector{10.0f, 0.0f, -13.0f},
		FVector{0.2f, 0.2f, 0.09f}}, GunMetal);
}