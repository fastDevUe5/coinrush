// Canvas-only HUD: no UMG widgets, no textures. Just text on top of the 3D view.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CRHUD.generated.h"

class UFont;

UCLASS()
class COINRUSH_API ACRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Color);
	void DrawLabel(const FString& Text, float X, float Y, UFont* Font, const FLinearColor& Color, float Scale = 1.f);
	void DrawLabelCentered(const FString& Text, float CenterX, float Y, UFont* Font, const FLinearColor& Color, float Scale = 1.f);
	float MeasureWidth(const FString& Text, UFont* Font, float Scale) const;

	/** Score "pop": the coin counter scales up briefly whenever it changes. */
	int32 LastScore = -1;
	float ScorePulseStart = -100.f;
};
