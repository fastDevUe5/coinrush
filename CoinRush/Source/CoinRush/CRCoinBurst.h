// Asset-free pickup burst: a handful of tiny emissive spheres flying outward.
// Spawned by ACRCoin when collected; lives ~0.45s and destroys itself.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CRCoinBurst.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class COINRUSH_API ACRCoinBurst : public AActor
{
	GENERATED_BODY()

public:
	ACRCoinBurst();

	virtual void Tick(float DeltaSeconds) override;

	static constexpr int32 NumShards = 8;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UPointLightComponent> Flash;

	TArray<FVector> Velocities;

	float LifeSeconds = 0.55f;
	float Age = 0.f;
};
