// Persistent player record (best score / best streak) stored in a save slot.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CRSaveGame.generated.h"

UCLASS()
class COINRUSH_API UCRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	int32 BestScore = 0;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	int32 BestStreak = 0;
};
