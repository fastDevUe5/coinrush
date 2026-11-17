// Coin Rush - a deliberately tiny 3D collect-the-coins game.
// All gameplay lives in C++ so the project has no blueprint/asset dependencies.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CRGameMode.generated.h"

/**
 * Round rules: 8 coins, 75 seconds, collect them all to win.
 */
UCLASS()
class COINRUSH_API ACRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACRGameMode();

	virtual void StartPlay() override;

	/** Called by a coin the moment the player touches it. */
	void AddCoin();

	/** Reloads the current map for a fresh round. */
	void RestartRound();

	UFUNCTION(BlueprintPure, Category = "CoinRush")
	bool IsRoundFinished() const { return bRoundFinished; }

	UFUNCTION(BlueprintPure, Category = "CoinRush")
	bool DidWin() const { return bWin; }

	UFUNCTION(BlueprintPure, Category = "CoinRush")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintPure, Category = "CoinRush")
	int32 GetTotalCoins() const { return TotalCoins; }

	UFUNCTION(BlueprintPure, Category = "CoinRush")
	float GetTimeLeft() const { return TimeLeft; }

	UFUNCTION(BlueprintPure, Category = "CoinRush")
	float GetRoundSeconds() const { return RoundSeconds; }

protected:
	void SpawnCoins();
	void TickSecond();
	void FinishRound(bool bInWin);

	/** Length of one round, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoinRush")
	float RoundSeconds = 75.f;

private:
	FTimerHandle SecondTimerHandle;
	int32 Score = 0;
	int32 TotalCoins = 0;
	float TimeLeft = 0.f;
	bool bRoundFinished = false;
	bool bWin = false;
};
