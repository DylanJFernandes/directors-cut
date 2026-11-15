#include "Game/GreyboxShapeLibrary.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace GreyboxShapeLibrary
{
	/* Name of the color parameter on both greybox materials */
	const FName ColorParameterName{TEXT("Color")};

	/* Asset path of the optional self-lit greybox material */
	const TCHAR* GlowMaterialPackagePath{
		TEXT("/Game/Materials/M_GreyboxGlow")};

	/* Applies a colored instance of the given material */
	UMaterialInstanceDynamic* ApplyColoredMaterial(
		UStaticMeshComponent* MeshComponent,
		UMaterialInterface* BaseMaterial,
		const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* ColoredMaterial{
			UMaterialInstanceDynamic::Create(BaseMaterial,
				MeshComponent)};
		ColoredMaterial->SetVectorParameterValue(
			ColorParameterName, Color);
		MeshComponent->SetMaterial(0, ColoredMaterial);
		return ColoredMaterial;
	}

	/* Returns the engine material every basic shape uses */
	UMaterialInterface* GetFlatMaterial()
	{
		return LoadObject<UMaterialInterface>(nullptr, TEXT(
			"/Engine/BasicShapes/BasicShapeMaterial"
			".BasicShapeMaterial"));
	}

	/* Returns the self-lit material, or null when the project
	   does not contain it */
	UMaterialInterface* GetGlowMaterial()
	{
		if (!FPackageName::DoesPackageExist(
			GlowMaterialPackagePath))
		{
			return nullptr;
		}
		return LoadObject<UMaterialInterface>(nullptr, TEXT(
			"/Game/Materials/M_GreyboxGlow.M_GreyboxGlow"));
	}
}

UStaticMesh* GreyboxShapeLibrary::GetCubeMesh()
{
	return LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UStaticMesh* GreyboxShapeLibrary::GetSphereMesh()
{
	return LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
}

UStaticMesh* GreyboxShapeLibrary::GetCylinderMesh()
{
	return LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

UMaterialInstanceDynamic* GreyboxShapeLibrary::ApplyFlatColor(
	UStaticMeshComponent* MeshComponent,
	const FLinearColor& Color)
{
	return ApplyColoredMaterial(MeshComponent, GetFlatMaterial(),
		Color);
}

UMaterialInstanceDynamic* GreyboxShapeLibrary::ApplyGlowColor(
	UStaticMeshComponent* MeshComponent,
	const FLinearColor& Color)
{
	UMaterialInterface* GlowMaterial{GetGlowMaterial()};
	if (!GlowMaterial)
	{
		return ApplyFlatColor(MeshComponent, Color);
	}
	return ApplyColoredMaterial(MeshComponent, GlowMaterial,
		Color);
}
