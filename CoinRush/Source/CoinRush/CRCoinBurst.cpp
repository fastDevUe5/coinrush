#include "CRCoinBurst.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ACRCoinBurst::ACRCoinBurst()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Root);
	Flash->SetIntensity(900.f);
	Flash->SetAttenuationRadius(260.f);
	Flash->SetLightColor(FLinearColor(1.f, 0.72f, 0.2f));
	Flash->SetCastShadows(false);
	Flash->SetMobility(EComponentMobility::Movable);

	Shards.Reserve(NumShards);
	Velocities.Reserve(NumShards);
}

void ACRCoinBurst::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[CoinRush] Collect burst spawned"));

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Coin.M_Coin"));

	for (int32 Index = 0; Index < NumShards; ++Index)
	{
		UStaticMeshComponent* Shard = NewObject<UStaticMeshComponent>(this);
		Shard->SetupAttachment(Root);
		Shard->RegisterComponent();
		Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shard->SetCanEverAffectNavigation(false);
		Shard->SetRelativeScale3D(FVector(0.11f));

		if (Sphere)
		{
			Shard->SetStaticMesh(Sphere);
		}
		if (Material)
		{
			Shard->SetMaterial(0, Material);
		}

		Shards.Add(Shard);

		FVector Velocity = FMath::VRand() * FMath::FRandRange(160.f, 380.f);
		Velocity.Z = FMath::Abs(Velocity.Z) + 80.f; // bias upward so the burst reads
		Velocities.Add(Velocity);
	}
}

void ACRCoinBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float Alpha = FMath::Clamp(Age / LifeSeconds, 0.f, 1.f);
	const float ShardScale = 0.11f * (1.f - Alpha);

	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		UStaticMeshComponent* Shard = Shards[Index];
		if (!Shard)
		{
			continue;
		}

		FVector& Velocity = Velocities[Index];
		Velocity.Z -= 700.f * DeltaSeconds;
		Shard->SetRelativeLocation(Shard->GetRelativeLocation() + Velocity * DeltaSeconds);
		Shard->SetRelativeScale3D(FVector(ShardScale));
	}

	Flash->SetIntensity(900.f * (1.f - Alpha));

	if (Age >= LifeSeconds)
	{
		Destroy();
	}
}
