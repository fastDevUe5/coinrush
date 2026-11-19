// Pure, engine-independent high score record so it can be unit tested.

#pragma once

#include "CoreMinimal.h"

struct FCRScoreRecord
{
	int32 BestScore = 0;
	int32 BestStreak = 0;

	/** Returns true when the result improved the stored record (either field). */
	bool Submit(int32 Score, int32 Streak)
	{
		bool bImproved = false;

		if (Score > BestScore)
		{
			BestScore = Score;
			bImproved = true;
		}

		if (Streak > BestStreak)
		{
			BestStreak = Streak;
			bImproved = true;
		}

		return bImproved;
	}
};
