#include "Misc/AutomationTest.h"

#include "CRRoundModel.h"

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
		TestFalse(FString::Printf(TEXT("coin %d does not finish the round"), Index + 1), Round.AddCoin());
	}

	TestEqual(TEXT("score after 7 coins"), Round.Score, 7);
	TestFalse(TEXT("still running after 7 coins"), Round.bFinished);

	TestTrue(TEXT("8th coin finishes the round"), Round.AddCoin());
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

	TestTrue(TEXT("only coin finishes"), Round.AddCoin());
	TestEqual(TEXT("score is 1"), Round.Score, 1);

	TestFalse(TEXT("late coin ignored"), Round.AddCoin());
	TestEqual(TEXT("score unchanged"), Round.Score, 1);
	TestFalse(TEXT("late tick ignored"), Round.TickSecond());
	TestEqual(TEXT("time left unchanged after finish"), Round.TimeLeft, 30.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
