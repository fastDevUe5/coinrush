#include "CRGameMode.h"

#include "CRCharacter.h"
#include "CRCoin.h"
#include "CRHUD.h"
#include "CRSaveGame.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

const TCHAR* ACRGameMode::SaveSlot = TEXT("CoinRushProfile");

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

	CoinSfx = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Coin.SFX_Coin"));
	LandSfx = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Land.SFX_Land"));
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] SFX loaded: coin=%d land=%d"),
		CoinSfx != nullptr ? 1 : 0, LandSfx != nullptr ? 1 : 0);

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

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const int32 PreviousScore = Round.Score;
	const bool bCompleted = Round.AddCoin(Now);

	if (Round.Score == PreviousScore)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Coin %d/%d"), Round.Score, Round.TotalCoins);

	if (Round.Streak >= 2)
	{
		// 连击奖励时间，给玩家"继续跑"的正反馈。
		UE_LOG(LogTemp, Log, TEXT("[CoinRush] Streak x%d (+%.1fs)"), Round.Streak, FCRRoundModel::StreakTimeBonus);
	}

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

	bNewRecord = Record.Submit(Round.Score, Round.BestStreak);
	if (bNewRecord)
	{
		SaveRecord();
	}
	else
	{
		// Nothing to write, but the stored record is still intact.
		bRecordSaved = true;
		SaveDiag.Empty();
	}

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Round over. Win=%d Score=%d/%d Record=%d NewRecord=%d"),
		bInWin ? 1 : 0, Round.Score, Round.TotalCoins, Record.BestScore, bNewRecord ? 1 : 0);
}

void ACRGameMode::BeginPlay()
{
	Super::BeginPlay();
	LoadRecord();
}

void ACRGameMode::LoadRecord()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlot, 0))
	{
		SaveGame = Cast<UCRSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
	}

	// Packaged builds may be denied %LOCALAPPDATA%; LoadRecord/SaveRecord then use the
	// portable copy stored next to the game.
	if (!SaveGame)
	{
		const FString PortablePath = FPaths::ProjectDir() / TEXT("Saved/SaveGames/") + FString(SaveSlot) + TEXT(".sav");
		TArray<uint8> Data;
		if (FFileHelper::LoadFileToArray(Data, *PortablePath) && Data.Num() > 0)
		{
			SaveGame = Cast<UCRSaveGame>(UGameplayStatics::LoadGameFromMemory(Data));
		}
	}

	if (!SaveGame)
	{
		SaveGame = Cast<UCRSaveGame>(UGameplayStatics::CreateSaveGameObject(UCRSaveGame::StaticClass()));
	}

	if (SaveGame)
	{
		Record.BestScore = SaveGame->BestScore;
		Record.BestStreak = SaveGame->BestStreak;
	}

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Record loaded: score=%d streak=%d"), Record.BestScore, Record.BestStreak);
}

void ACRGameMode::SaveRecord()
{
	if (!SaveGame)
	{
		bRecordSaved = false;
		SaveDiag = TEXT("no save object");
		UE_LOG(LogTemp, Warning, TEXT("[CoinRush] Record save skipped: no save game object"));
		return;
	}

	SaveGame->BestScore = Record.BestScore;
	SaveGame->BestStreak = Record.BestStreak;

	// Do the two steps manually so a packaged build (no logs) can show where it failed.
	TArray<uint8> Data;
	const bool bSerialized = UGameplayStatics::SaveGameToMemory(SaveGame, Data);

	// Packaged builds resolve ProjectSavedDir() to %LOCALAPPDATA%\<Project>\Saved, which some
	// machines deny to unsigned binaries; fall back to <Package>\Saved so progress is never lost.
	const FString Candidates[] =
	{
		GetSaveDirectory(),
		FPaths::ProjectDir() / TEXT("Saved/SaveGames/"),
	};

	FString UsedDirectory;
	bool bWritten = false;
	uint32 LastError = 0;
	for (const FString& Directory : Candidates)
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FString Path = Directory + FString(SaveSlot) + TEXT(".sav");
		if (bSerialized && Data.Num() > 0 && FFileHelper::SaveArrayToFile(Data, *Path))
		{
			bWritten = true;
			UsedDirectory = Directory;
			break;
		}
		LastError = FPlatformMisc::GetLastError();
	}

	bRecordSaved = bWritten;
	SaveDiag = FString::Printf(TEXT("mem=%d bytes=%d write=%d err=%u dir=%s"),
		bSerialized ? 1 : 0, Data.Num(), bWritten ? 1 : 0, bWritten ? 0u : LastError,
		*(bWritten ? UsedDirectory : GetSaveDirectory()));
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Record saved=%d: score=%d streak=%d %s"),
		bRecordSaved ? 1 : 0, Record.BestScore, Record.BestStreak, *SaveDiag);
}

FString ACRGameMode::GetSaveDirectory() const
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames/");
}

void ACRGameMode::RestartRound()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void ACRGameMode::PlayCoinSfx(const FVector& Location)
{
	if (!CoinSfx)
	{
		return;
	}

	// Slight random pitch so repeated pickups do not sound robotic.
	UGameplayStatics::PlaySoundAtLocation(this, CoinSfx, Location, 0.7f, FMath::FRandRange(0.95f, 1.06f));
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] SFX coin"));
}

void ACRGameMode::PlayLandSfx()
{
	if (!LandSfx)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, LandSfx, 0.55f, FMath::FRandRange(0.92f, 1.08f));
}

void ACRGameMode::AddBonusTime(float Seconds)
{
	if (Round.bFinished || Seconds <= 0.f)
	{
		return;
	}

	Round.AddTime(Seconds);
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Bonus time +%.1fs (time left %.1fs)"), Seconds, Round.TimeLeft);
}

void ACRGameMode::ApplyTimePenalty(float Seconds)
{
	if (Round.bFinished || Seconds <= 0.f)
	{
		return;
	}

	Round.AddTime(-Seconds);
	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Time penalty -%.1fs (time left %.1fs)"), Seconds, Round.TimeLeft);

	if (Round.TimeLeft <= 0.f)
	{
		FinishRound(false);
	}
}
