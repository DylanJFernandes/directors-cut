#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

/* Shared access to the engine basic shapes and flat colors used
   for every greybox placeholder in the prototype */
namespace GreyboxShapeLibrary
{
	/* Edge length of the engine basic shapes, in units */
	constexpr float BasicShapeSize{100.0f};

	/* Returns the engine cube mesh */
	UStaticMesh* GetCubeMesh();

	/* Returns the engine sphere mesh */
	UStaticMesh* GetSphereMesh();

	/* Returns the engine cylinder mesh */
	UStaticMesh* GetCylinderMesh();

	/* Gives the mesh component a flat colored material and
	   returns that material so the color can change later */
	UMaterialInstanceDynamic* ApplyFlatColor(
		UStaticMeshComponent* MeshComponent,
		const FLinearColor& Color);

	/* Gives the mesh component a self-lit colored material when
	   the glow material asset exists, else a flat color */
	UMaterialInstanceDynamic* ApplyGlowColor(
		UStaticMeshComponent* MeshComponent,
		const FLinearColor& Color);
}
