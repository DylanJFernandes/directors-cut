#pragma once

#include "CoreMinimal.h"

/* Plays the prototype's few placeholder sounds. Each sound is
   an optional asset under /Game/Audio; when it is missing the
   call does nothing. */
namespace BacklotSoundLibrary
{
	/* Plays the megaphone shout at a location */
	void PlayMegaphone(const UObject* WorldContextObject,
		const FVector& Location);

	/* Plays one tommy gun shot at a location */
	void PlayTommyGunShot(const UObject* WorldContextObject,
		const FVector& Location);

	/* Plays one guard gunshot at a location */
	void PlayGuardShot(const UObject* WorldContextObject,
		const FVector& Location);

	/* Plays the clapperboard snap of a takedown at a location */
	void PlayClapperboard(const UObject* WorldContextObject,
		const FVector& Location);
}