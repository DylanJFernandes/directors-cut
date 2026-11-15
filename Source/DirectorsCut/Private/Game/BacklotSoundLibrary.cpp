#include "Game/BacklotSoundLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

namespace BacklotSoundLibrary
{
	/* Returns the sound asset with the given name in
	   /Game/Audio, or null when the project does not have it */
	USoundBase* FindSound(const FString& AssetName)
	{
		const FString PackagePath{TEXT("/Game/Audio/") + AssetName};
		const FString ObjectPath{PackagePath + TEXT(".") + AssetName};
		if (USoundBase* LoadedSound{
			FindObject<USoundBase>(nullptr, *ObjectPath)})
		{
			return LoadedSound;
		}
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}
		return LoadObject<USoundBase>(nullptr, *ObjectPath);
	}

	/* Plays the named sound at a location when it exists */
	void PlayNamedSound(const UObject* WorldContextObject,
		const FString& AssetName, const FVector& Location,
		float VolumeMultiplier)
	{
		if (USoundBase* Sound{FindSound(AssetName)})
		{
			UGameplayStatics::PlaySoundAtLocation(
				WorldContextObject, Sound, Location,
				VolumeMultiplier);
		}
	}
}

void BacklotSoundLibrary::PlayMegaphone(
	const UObject* WorldContextObject, const FVector& Location)
{
	PlayNamedSound(WorldContextObject, TEXT("S_Megaphone"),
		Location, 1.0f);
}

void BacklotSoundLibrary::PlayTommyGunShot(
	const UObject* WorldContextObject, const FVector& Location)
{
	PlayNamedSound(WorldContextObject, TEXT("S_TommyGunShot"),
		Location, 0.6f);
}

void BacklotSoundLibrary::PlayGuardShot(
	const UObject* WorldContextObject, const FVector& Location)
{
	PlayNamedSound(WorldContextObject, TEXT("S_GuardShot"),
		Location, 0.8f);
}

void BacklotSoundLibrary::PlayClapperboard(
	const UObject* WorldContextObject, const FVector& Location)
{
	PlayNamedSound(WorldContextObject, TEXT("S_Clapperboard"),
		Location, 1.0f);
}