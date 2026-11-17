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

	/** 连击：两次拾取间隔小于 StreakWindow 就累加。 */
	int32 Streak = 0;
	int32 BestStreak = 0;

	/** 连击窗口（秒）与每枚连击金币奖励的时间。
	 *  金币间距约 1100cm：走路 2.4s、冲刺 1.3s，取 4s 让人"稍微规划路线"就能连上。 */
	static constexpr float StreakWindowSeconds = 4.0f;
	static constexpr float StreakTimeBonus = 1.5f;

	void Reset(float InRoundSeconds)
	{
		TotalCoins = 0;
		Score = 0;
		TimeLeft = FMath::Max(0.f, InRoundSeconds);
		bFinished = false;
		bWon = false;
		Streak = 0;
		BestStreak = 0;
		LastCoinTime = -1000.f;
	}

	void SetTotalCoins(int32 Count)
	{
		TotalCoins = FMath::Max(0, Count);
	}

	/**
	 * Registers one collected coin.
	 * @param NowSeconds  World time of the pickup, used for the streak window.
	 * @return true when this coin completes the round.
	 */
	bool AddCoin(float NowSeconds = -1000.f)
	{
		if (bFinished)
		{
			return false;
		}

		Streak = (NowSeconds - LastCoinTime <= StreakWindowSeconds) ? Streak + 1 : 1;
		LastCoinTime = NowSeconds;
		BestStreak = FMath::Max(BestStreak, Streak);

		// Chained pickups buy time back; the first coin of a chain does not.
		if (Streak >= 2)
		{
			TimeLeft += StreakTimeBonus;
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

private:
	float LastCoinTime = -1000.f;
};
