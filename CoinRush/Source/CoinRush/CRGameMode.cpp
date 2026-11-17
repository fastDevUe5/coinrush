#include "CRGameMode.h"

#include "CRCharacter.h"
#include "CRCoin.h"
#include "CRHUD.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	/** Coin positions, in world centimetres. They intentionally mirror the arena built by Tools/build_level.py. */
	struct FCoinSpot
	{
		float X;
		float Y;
		float Z;
	};

	const FCoinSpot GCoinSpots[] =
	{
		{    0.f,     0.f, 235.f }, // on the central platform
		{    0.f, -1150.f, 125.f }, // right in front of the spawn point
		{ 1000.f,  -620.f, 125.f },
		{ -1000.f, -620.f, 125.f },
		{ 1400.f,  1400.f, 235.f }, // side platforms
		{ -1400.f, 1400.f, 235.f },
		{ 1250.f,     0.f, 125.f },
		{ -1250.f,    0.f, 125.f },
	};
}

ACRGameMode::ACRGameMode()
{
	DefaultPawnClass = ACRCharacter::StaticClass();
	HUDClass = ACRHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = false;
}

void ACRGameMode::StartPlay()
{
	Super::StartPlay();

	// 引擎默认平滑帧率把上限压到 62fps；这里提到 120（用户可用 -ExecCmds 覆盖）。
	// 放在代码里而不是 DefaultEngine.ini，是因为 [SystemSettings] 的 t.MaxFPS 实测不生效。
	if (IConsoleVariable* MaxFPS = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
	{
		if (MaxFPS->GetFloat() <= 0.f)
		{
			MaxFPS->Set(120.f);
		}
	}

	Round.Reset(RoundSeconds);

	SpawnCoins();

	GetWorldTimerManager().SetTimer(SecondTimerHandle, this, &ACRGameMode::TickSecond, 1.f, true);
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Round started: %d coins, %.0f seconds."), Round.TotalCoins, RoundSeconds);
}

void ACRGameMode::SpawnCoins()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 SpawnedCoins = 0;
	for (const FCoinSpot& Spot : GCoinSpots)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (World->SpawnActor<ACRCoin>(ACRCoin::StaticClass(), FVector(Spot.X, Spot.Y, Spot.Z), FRotator::ZeroRotator, SpawnParams))
		{
			++SpawnedCoins;
		}
	}

	Round.SetTotalCoins(SpawnedCoins);
}

void ACRGameMode::AddCoin()
{
	if (Round.bFinished)
	{
		return;
	}

	const int32 PreviousScore = Round.Score;
	const bool bCompleted = Round.AddCoin();

	if (Round.Score == PreviousScore)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Coin %d/%d"), Round.Score, Round.TotalCoins);

	if (bCompleted)
	{
		FinishRound(true);
	}
}

void ACRGameMode::TickSecond()
{
	if (Round.bFinished)
	{
		return;
	}

	if (Round.TickSecond())
	{
		FinishRound(false);
	}
}

void ACRGameMode::FinishRound(bool bInWin)
{
	// 状态由 FCRRoundModel 在 AddCoin/TickSecond 里置位，这里只负责收尾。
	GetWorldTimerManager().ClearTimer(SecondTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Round over. Win=%d Score=%d/%d"), bInWin ? 1 : 0, Round.Score, Round.TotalCoins);
}

void ACRGameMode::RestartRound()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
