#include "CRCoin.h"

#include "CRCharacter.h"
#include "CRCoinBurst.h"
#include "CRGameMode.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACRCoin::ACRCoin()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(62.f);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCanEverAffectNavigation(false);
	SetRootComponent(Collision);

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(Collision);
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoinMesh->SetCanEverAffectNavigation(false);
	CoinMesh->SetRelativeScale3D(FVector(0.46f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CoinMesh->SetStaticMesh(SphereMesh.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetIntensity(420.f);
	Glow->SetAttenuationRadius(320.f);
	Glow->SetLightColor(FLinearColor(1.f, 0.62f, 0.12f));
	Glow->SetCastShadows(false);
	Glow->SetMobility(EComponentMobility::Movable);
}

void ACRCoin::BeginPlay()
{
	Super::BeginPlay();

	BaseLocation = GetActorLocation();
	Phase = FMath::FRandRange(0.f, 2.f * PI);

	// Loaded at begin play so a project without generated materials still runs.
	if (UMaterialInterface* CoinMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Coin.M_Coin")))
	{
		CoinMesh->SetMaterial(0, CoinMaterial);
	}

	Collision->OnComponentBeginOverlap.AddDynamic(this, &ACRCoin::OnSphereBeginOverlap);
}

void ACRCoin::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Collected: play a short pop (grow, then vanish) instead of disappearing instantly.
	if (bCollected)
	{
		PopTime -= DeltaSeconds;
		const float Alpha = 1.f - FMath::Clamp(PopTime / FMath::Max(PopDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		SetActorScale3D(FVector(1.f + 1.1f * Alpha));

		if (PopTime <= 0.f)
		{
			Destroy();
		}
		return;
	}

	AddActorLocalRotation(FRotator(0.f, SpinSpeed * DeltaSeconds, 0.f));

	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	SetActorLocation(BaseLocation + FVector(0.f, 0.f, FMath::Sin(Time * 2.2f + Phase) * BobAmplitude));
}

void ACRCoin::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected || !OtherActor || !OtherActor->IsA<ACRCharacter>())
	{
		return;
	}

	bCollected = true;
	PopTime = PopDuration;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glow->SetIntensity(1800.f);
	Glow->SetAttenuationRadius(500.f);

	if (UWorld* World = GetWorld())
	{
		World->SpawnActor<ACRCoinBurst>(ACRCoinBurst::StaticClass(), GetActorLocation(), FRotator::ZeroRotator);
	}

	if (ACRGameMode* GameMode = Cast<ACRGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->AddCoin();
	}
}
