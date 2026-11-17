#include "CRHUD.h"

#include "CRGameMode.h"
#include "CRCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

/** Toggle with:  cr.DebugInfo 1   (useful while tuning the camera or arena). */
static bool GCoinRushDebugInfo = false;
static FAutoConsoleVariableRef CVarCoinRushDebugInfo(
	TEXT("cr.DebugInfo"),
	GCoinRushDebugInfo,
	TEXT("Draw pawn / camera diagnostics on the CoinRush HUD."),
	ECVF_Default);

void ACRHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !GEngine)
	{
		return;
	}

	ACRGameMode* GameMode = Cast<ACRGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode)
	{
		return;
	}

	UFont* LargeFont = GEngine->GetLargeFont();
	UFont* SmallFont = GEngine->GetMediumFont();
	if (!LargeFont || !SmallFont)
	{
		return;
	}

	const float ViewW = Canvas->SizeX;
	const float ViewH = Canvas->SizeY;
	const float UIScale = FMath::Clamp(ViewH / 720.f, 0.75f, 2.5f);

	const FLinearColor Gold(1.f, 0.76f, 0.22f, 1.f);
	const FLinearColor Paper(0.93f, 0.96f, 1.f, 1.f);
	const FLinearColor Danger(1.f, 0.30f, 0.24f, 1.f);

	// Top bar with the two numbers that matter.
	DrawPanel(0.f, 0.f, ViewW, 88.f * UIScale, FLinearColor(0.02f, 0.03f, 0.06f, 0.55f));

	const FString CoinText = FString::Printf(TEXT("COINS   %d / %d"), GameMode->GetScore(), GameMode->GetTotalCoins());

	// Pulse the counter whenever the score changes (0.35s ease-out).
	const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	if (GameMode->GetScore() != LastScore)
	{
		LastScore = GameMode->GetScore();
		ScorePulseStart = Now;
	}
	const float ScorePulse = FMath::Clamp(1.f - (Now - ScorePulseStart) / 0.35f, 0.f, 1.f);
	const float ScoreScale = 1.25f + 0.55f * ScorePulse * ScorePulse;

	DrawLabel(CoinText, 42.f * UIScale, 22.f * UIScale, LargeFont, Gold, ScoreScale * UIScale);

	// 连击：只在 >=2 时显示，提示"继续快跑能换时间"。
	if (GameMode->GetStreak() >= 2)
	{
		const FString StreakText = FString::Printf(TEXT("STREAK  x%d    +%.1fs"), GameMode->GetStreak(), FCRRoundModel::StreakTimeBonus);
		const float StreakAlpha = FMath::Clamp(ScorePulse * 1.2f, 0.f, 1.f);
		const FLinearColor StreakColor = FLinearColor::LerpUsingHSV(Paper, Gold, StreakAlpha);
		DrawLabel(StreakText, 42.f * UIScale, 62.f * UIScale, SmallFont, StreakColor, 1.0f * UIScale);
	}

	const int32 WholeSeconds = FMath::CeilToInt(GameMode->GetTimeLeft());
	const FString TimeText = FString::Printf(TEXT("%02d"), WholeSeconds);
	const FLinearColor TimeColor = (WholeSeconds <= 10) ? Danger : Paper;
	DrawLabelCentered(TimeText, ViewW * 0.5f, 14.f * UIScale, LargeFont, TimeColor, 1.9f * UIScale);
	DrawLabelCentered(TEXT("SECONDS LEFT"), ViewW * 0.5f, 22.f * UIScale + 40.f * UIScale, SmallFont, TimeColor * 0.75f, 0.85f * UIScale);

	// Sprint indicator on the right edge of the top bar.
	const ACRCharacter* Player = Cast<ACRCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const FString SprintText = (Player && Player->IsSprinting()) ? TEXT("SPRINT") : TEXT("SHIFT = SPRINT");
	const FLinearColor SprintColor = (Player && Player->IsSprinting()) ? Gold : FLinearColor(0.55f, 0.62f, 0.75f, 1.f);
	DrawLabel(SprintText, ViewW - MeasureWidth(SprintText, SmallFont, 0.95f * UIScale) - 42.f * UIScale, 24.f * UIScale, SmallFont, SprintColor, 0.95f * UIScale);

	// Controls reminder.
	DrawLabelCentered(TEXT("WASD  MOVE     SPACE  JUMP     SHIFT  SPRINT     Q / E  TURN     R / F  PITCH     ENTER  RESTART"),
		ViewW * 0.5f, ViewH - 40.f * UIScale, SmallFont, FLinearColor(0.62f, 0.68f, 0.78f, 0.9f), 0.85f * UIScale);

	if (GCoinRushDebugInfo)
	{
		FString Info = TEXT("no pawn");
		if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector PawnLoc = Pawn->GetActorLocation();
			Info = FString::Printf(TEXT("PAWN %.0f %.0f %.0f"), PawnLoc.X, PawnLoc.Y, PawnLoc.Z);
			if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				if (PC->PlayerCameraManager)
				{
					const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
					const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
					Info += FString::Printf(TEXT("    CAM %.0f %.0f %.0f    YAW %.0f PITCH %.0f"),
						CamLoc.X, CamLoc.Y, CamLoc.Z, CamRot.Yaw, CamRot.Pitch);
				}
			}
		}
		DrawLabel(Info, 20.f, ViewH - 74.f * UIScale, SmallFont, FLinearColor(0.7f, 1.f, 0.75f, 1.f), 0.85f * UIScale);
	}

	if (!GameMode->IsRoundFinished())
	{
		// Aiming ring in the middle of the screen, drawn in gold.
		const float CX = ViewW * 0.5f;
		const float CY = ViewH * 0.5f;
		DrawPanel(CX - 1.f * UIScale, CY - 7.f * UIScale, 2.f * UIScale, 14.f * UIScale, FLinearColor(1.f, 0.85f, 0.35f, 0.6f));
		DrawPanel(CX - 7.f * UIScale, CY - 1.f * UIScale, 14.f * UIScale, 2.f * UIScale, FLinearColor(1.f, 0.85f, 0.35f, 0.6f));
		return;
	}

	// End of round overlay.
	DrawPanel(0.f, 0.f, ViewW, ViewH, FLinearColor(0.01f, 0.015f, 0.03f, 0.62f));

	const bool bWin = GameMode->DidWin();
	const FString Headline = bWin ? TEXT("ALL COINS COLLECTED!") : TEXT("TIME UP!");
	const FLinearColor HeadlineColor = bWin ? Gold : Danger;
	DrawLabelCentered(Headline, ViewW * 0.5f, ViewH * 0.5f - 110.f * UIScale, LargeFont, HeadlineColor, 2.1f * UIScale);

	const FString Summary = FString::Printf(TEXT("COINS   %d / %d      BEST STREAK   x%d"),
		GameMode->GetScore(), GameMode->GetTotalCoins(), GameMode->GetBestStreak());
	DrawLabelCentered(Summary, ViewW * 0.5f, ViewH * 0.5f - 10.f * UIScale, LargeFont, Paper, 1.1f * UIScale);

	DrawLabelCentered(TEXT("PRESS ENTER TO PLAY AGAIN     ESC TO QUIT"), ViewW * 0.5f, ViewH * 0.5f + 90.f * UIScale, SmallFont,
		FLinearColor(0.7f, 0.76f, 0.85f, 1.f), 1.0f * UIScale);
}

void ACRHUD::DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Color)
{
	if (!Canvas)
	{
		return;
	}

	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(Width, Height), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void ACRHUD::DrawLabel(const FString& Text, float X, float Y, UFont* Font, const FLinearColor& Color, float Scale)
{
	if (!Canvas || !Font)
	{
		return;
	}

	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	Canvas->DrawItem(Item);
}

void ACRHUD::DrawLabelCentered(const FString& Text, float CenterX, float Y, UFont* Font, const FLinearColor& Color, float Scale)
{
	DrawLabel(Text, CenterX - MeasureWidth(Text, Font, Scale) * 0.5f, Y, Font, Color, Scale);
}

float ACRHUD::MeasureWidth(const FString& Text, UFont* Font, float Scale) const
{
	if (!Canvas || !Font)
	{
		return 0.f;
	}

	float Width = 0.f;
	float Height = 0.f;
	Canvas->StrLen(Font, Text, Width, Height);
	return Width * Scale;
}
