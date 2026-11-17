// A glowing collectible coin: spins, bobs and reports itself to the game mode on touch.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CRCoin.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class COINRUSH_API ACRCoin : public AActor
{
	GENERATED_BODY()

public:
	ACRCoin();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UStaticMeshComponent> CoinMesh;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(EditAnywhere, Category = "CoinRush")
	float SpinSpeed = 140.f;

	UPROPERTY(EditAnywhere, Category = "CoinRush")
	float BobAmplitude = 16.f;

	/** How long the collect "pop" (scale up, then vanish) takes. */
	UPROPERTY(EditAnywhere, Category = "CoinRush")
	float PopDuration = 0.18f;

private:
	FVector BaseLocation = FVector::ZeroVector;
	float Phase = 0.f;
	float PopTime = 0.f;
	bool bCollected = false;
};
