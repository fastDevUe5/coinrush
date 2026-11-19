#include "Misc/AutomationTest.h"

#include "CRRoundModel.h"
#include "CRScoreRecord.h"

#if WITH_DEV_AUTOMATION_TESTS

// 用法：见 Tools/RunTests.ps1（或编辑器里的 Session Frontend → Automation）
// 这些测试不创建世界、不依赖引擎子系统，跑得很快，适合每次改动后回归。

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRoundModelCollectAllTest, "CoinRush.RoundModel.CollectAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRoundModelCollectAllTest::RunTest(const FString& Parameters)
{
	FCRRoundModel Round;
	Round.Reset(75.f);
	Round.SetTotalCoins(8);

	TestEqual(TEXT("total coins"), Round.TotalCoins, 8);
	TestEqual(TEXT("time left"), Round.TimeLeft, 75.f);
	TestFalse(TEXT("not finished at start"), Round.bFinished);

	for (int32 Index = 0; Index < 7; ++Index)
	{
		const float Now = static_cast<float>(Index) * 10.f; // far apart: no streak
		TestFalse(FString::Printf(TEXT("coin %d does not finish the round"), Index + 1), Round.AddCoin(Now));
	}

	TestEqual(TEXT("score after 7 coins"), Round.Score, 7);
	TestFalse(TEXT("still running after 7 coins"), Round.bFinished);

	TestTrue(TEXT("8th coin finishes the round"), Round.AddCoin(80.f));
	TestEqual(TEXT("score after 8 coins"), Round.Score, 8);
	TestTrue(TEXT("finished"), Round.bFinished);
	TestTrue(TEXT("won"), Round.bWon);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRoundModelTimeoutTest, "CoinRush.RoundModel.Timeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRoundModelTimeoutTest::RunTest(const FString& Parameters)
{
	FCRRoundModel Round;
	Round.Reset(2.f);
	Round.SetTotalCoins(8);

	TestFalse(TEXT("1s left: not finished"), Round.TickSecond());
	TestEqual(TEXT("time left after one tick"), Round.TimeLeft, 1.f);

	TestTrue(TEXT("0s left: finishes"), Round.TickSecond());
	TestEqual(TEXT("time clamped at zero"), Round.TimeLeft, 0.f);
	TestTrue(TEXT("finished"), Round.bFinished);
	TestFalse(TEXT("lost, not won"), Round.bWon);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRoundModelIgnoresLateCoinsTest, "CoinRush.RoundModel.IgnoreAfterFinish",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRoundModelIgnoresLateCoinsTest::RunTest(const FString& Parameters)
{
	FCRRoundModel Round;
	Round.Reset(30.f);
	Round.SetTotalCoins(1);

	TestTrue(TEXT("only coin finishes"), Round.AddCoin(1.f));
	TestEqual(TEXT("score is 1"), Round.Score, 1);

	TestFalse(TEXT("late coin ignored"), Round.AddCoin(2.f));
	TestEqual(TEXT("score unchanged"), Round.Score, 1);
	TestFalse(TEXT("late tick ignored"), Round.TickSecond());
	TestEqual(TEXT("time left unchanged after finish"), Round.TimeLeft, 30.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRoundModelStreakTest, "CoinRush.RoundModel.Streak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRoundModelStreakTest::RunTest(const FString& Parameters)
{
	FCRRoundModel Round;
	Round.Reset(30.f);
	Round.SetTotalCoins(8);

	// First coin starts the chain, no time bonus.
	Round.AddCoin(10.f);
	TestEqual(TEXT("streak after first coin"), Round.Streak, 1);
	TestEqual(TEXT("no bonus on first coin"), Round.TimeLeft, 30.f);

	// Second coin inside the window: streak 2, +1.5s.
	Round.AddCoin(11.f);
	TestEqual(TEXT("streak after quick second coin"), Round.Streak, 2);
	TestEqual(TEXT("bonus applied to time"), Round.TimeLeft, 31.5f);

	// Third coin inside the window: streak 3, +1.5s again.
	Round.AddCoin(12.f);
	TestEqual(TEXT("streak after third coin"), Round.Streak, 3);
	TestEqual(TEXT("second bonus applied"), Round.TimeLeft, 33.f);
	TestEqual(TEXT("best streak tracked"), Round.BestStreak, 3);

	// A slow coin resets the chain and gives no bonus.
	Round.AddCoin(20.f);
	TestEqual(TEXT("slow coin resets streak"), Round.Streak, 1);
	TestEqual(TEXT("no bonus after reset"), Round.TimeLeft, 33.f);
	TestEqual(TEXT("best streak kept"), Round.BestStreak, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRoundModelBonusTimeTest, "CoinRush.RoundModel.BonusTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRoundModelBonusTimeTest::RunTest(const FString& Parameters)
{
	FCRRoundModel Round;
	Round.Reset(10.f);
	Round.SetTotalCoins(8);

	Round.AddTime(2.5f);
	TestEqual(TEXT("bonus time added"), Round.TimeLeft, 12.5f);

	Round.AddTime(-100.f);
	TestEqual(TEXT("time never goes negative"), Round.TimeLeft, 0.f);

	FCRRoundModel FinishedRound;
	FinishedRound.Reset(1.f);
	FinishedRound.TickSecond(5.f);
	TestTrue(TEXT("finished by timeout"), FinishedRound.bFinished);
	FinishedRound.AddTime(10.f);
	TestEqual(TEXT("bonus ignored after the round ends"), FinishedRound.TimeLeft, 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRoundModelPenaltyTest, "CoinRush.RoundModel.TimePenalty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRoundModelPenaltyTest::RunTest(const FString& Parameters)
{
	FCRRoundModel Round;
	Round.Reset(10.f);

	Round.AddTime(-3.f);
	TestEqual(TEXT("penalty applied"), Round.TimeLeft, 7.f);

	Round.AddTime(-30.f);
	TestEqual(TEXT("penalty clamps at zero"), Round.TimeLeft, 0.f);
	TestFalse(TEXT("penalty alone does not finish the round"), Round.bFinished);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRScoreRecordTest, "CoinRush.ScoreRecord.Submit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRScoreRecordTest::RunTest(const FString& Parameters)
{
	FCRScoreRecord Record;

	TestTrue(TEXT("first score is a record"), Record.Submit(3, 2));
	TestEqual(TEXT("best score updated"), Record.BestScore, 3);
	TestEqual(TEXT("best streak updated"), Record.BestStreak, 2);

	TestFalse(TEXT("worse round is not a record"), Record.Submit(2, 1));
	TestTrue(TEXT("higher score is a record"), Record.Submit(5, 1));
	TestEqual(TEXT("streak kept when not beaten"), Record.BestStreak, 2);
	TestTrue(TEXT("better streak alone is a record"), Record.Submit(5, 4));
	TestEqual(TEXT("both fields tracked"), Record.BestStreak, 4);

	// A zero round against an empty record is not an improvement.
	FCRScoreRecord Empty;
	TestFalse(TEXT("zero does not beat zero"), Empty.Submit(0, 0));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
