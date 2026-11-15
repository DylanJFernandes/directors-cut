#pragma once

#include "CoreMinimal.h"

class USceneComponent;

/* Builds the greybox tommy gun: a body, stock and grip with a
   film reel for a barrel and another for the drum magazine */
namespace TommyGunShape
{
	/* Distance from the gun's origin to its muzzle */
	constexpr float MuzzleDistance{48.0f};

	/* Adds the gun's pieces under a component, pointing along
	   that component's forward axis */
	void Build(USceneComponent& Parent);
}