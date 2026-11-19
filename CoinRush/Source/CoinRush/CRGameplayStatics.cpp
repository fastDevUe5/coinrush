#include "CRGameplayStatics.h"

#include "CRGameMode.h"
#include "Kismet/GameplayStatics.h"

void UCRGameplayStatics::AddBonusTime(const UObject* WorldContextObject, float Seconds)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (ACRGameMode* GameMode = Cast<ACRGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		GameMode->AddBonusTime(Seconds);
	}
}

void UCRGameplayStatics::ApplyTimePenalty(const UObject* WorldContextObject, float Seconds)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (ACRGameMode* GameMode = Cast<ACRGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		GameMode->ApplyTimePenalty(Seconds);
	}
}
