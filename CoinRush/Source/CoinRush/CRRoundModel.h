// Pure round state. No UObject, no world, no timer: kept separate so it can be
// unit tested headlessly (see Private/Tests/CRRoundModelTests.cpp).

#pragma once

#include "CoreMinimal.h"

struct COINRUSH_API FCRRoundModel
{
	int32 TotalCoins = 0;
	int32 Score = 0;
	float TimeLeft = 0.f;
	bool bFinished = false;
	bool bWon = false;

	void Reset(float InRoundSeconds)
	{
		TotalCoins = 0;
		Score = 0;
		TimeLeft = FMath::Max(0.f, InRoundSeconds);
		bFinished = false;
		bWon = false;
	}

	void SetTotalCoins(int32 Count)
	{
		TotalCoins = FMath::Max(0, Count);
	}

	/** Returns true when this coin completes the round. Coins after the end are ignored. */
	bool AddCoin()
	{
		if (bFinished)
		{
			return false;
		}

		++Score;

		if (TotalCoins > 0 && Score >= TotalCoins)
		{
			bFinished = true;
			bWon = true;
			return true;
		}

		return false;
	}

	/** Advances the clock by DeltaSeconds. Returns true when time runs out on this tick. */
	bool TickSecond(float DeltaSeconds = 1.f)
	{
		if (bFinished)
		{
			return false;
		}

		TimeLeft = FMath::Max(0.f, TimeLeft - DeltaSeconds);

		if (TimeLeft <= 0.f)
		{
			bFinished = true;
			bWon = false;
			return true;
		}

		return false;
	}
};
