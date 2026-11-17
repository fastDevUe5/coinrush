#include "CRGameMode.h"

#include "CRCharacter.h"
#include "CRCoin.h"
#include "CRHUD.h"
#include "Engine/World.h"
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

	Score = 0;
	TotalCoins = 0;
	TimeLeft = RoundSeconds;
	bRoundFinished = false;
	bWin = false;

	SpawnCoins();

	GetWorldTimerManager().SetTimer(SecondTimerHandle, this, &ACRGameMode::TickSecond, 1.f, true);
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Round started: %d coins, %.0f seconds."), TotalCoins, RoundSeconds);
}

void ACRGameMode::SpawnCoins()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FCoinSpot& Spot : GCoinSpots)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (World->SpawnActor<ACRCoin>(ACRCoin::StaticClass(), FVector(Spot.X, Spot.Y, Spot.Z), FRotator::ZeroRotator, SpawnParams))
		{
			++TotalCoins;
		}
	}
}

void ACRGameMode::AddCoin()
{
	if (bRoundFinished)
	{
		return;
	}

	++Score;
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Coin %d/%d"), Score, TotalCoins);

	if (TotalCoins > 0 && Score >= TotalCoins)
	{
		FinishRound(true);
	}
}

void ACRGameMode::TickSecond()
{
	if (bRoundFinished)
	{
		return;
	}

	TimeLeft = FMath::Max(0.f, TimeLeft - 1.f);

	if (TimeLeft <= 0.f)
	{
		FinishRound(false);
	}
}

void ACRGameMode::FinishRound(bool bInWin)
{
	if (bRoundFinished)
	{
		return;
	}

	bRoundFinished = true;
	bWin = bInWin;
	GetWorldTimerManager().ClearTimer(SecondTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Round over. Win=%d Score=%d/%d"), bInWin ? 1 : 0, Score, TotalCoins);
}

void ACRGameMode::RestartRound()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
