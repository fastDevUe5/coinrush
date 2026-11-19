// Small Blueprint-facing helpers so level props authored in Blueprints can talk to the
// C++ round rules without casting to the game mode.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CRGameplayStatics.generated.h"

UCLASS()
class COINRUSH_API UCRGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Grants extra seconds to the current round (used by Blueprint bonus pickups). */
	UFUNCTION(BlueprintCallable, Category = "CoinRush", meta = (WorldContext = "WorldContextObject"))
	static void AddBonusTime(const UObject* WorldContextObject, float Seconds);

	/** Removes seconds from the current round (used by Blueprint hazards). */
	UFUNCTION(BlueprintCallable, Category = "CoinRush", meta = (WorldContext = "WorldContextObject"))
	static void ApplyTimePenalty(const UObject* WorldContextObject, float Seconds);
};
