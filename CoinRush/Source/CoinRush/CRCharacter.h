// Third-person player character built entirely from engine primitives (no skeletal mesh needed).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CRCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class COINRUSH_API ACRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACRCharacter();

	virtual void Tick(float DeltaSeconds) override;

	/** True while Shift is held and the character is moving. */
	bool IsSprinting() const { return bSprinting; }

protected:
	virtual void BeginPlay() override;

	/** Landing thud: a short decaying camera dip. */
	virtual void Landed(const FHitResult& Hit) override;

private:
	void UpdateLook(float DeltaSeconds);
	void UpdateMove(float DeltaSeconds);
	void UpdateJump();
	void UpdateLandShake(float DeltaSeconds);
	void ReadRoundShortcuts();

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, Category = "CoinRush")
	TObjectPtr<UStaticMeshComponent> VisorMesh;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Movement")
	float WalkSpeed = 470.f;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Movement")
	float SprintSpeed = 860.f;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Camera")
	float CameraYawRate = 110.f;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Camera")
	float CameraPitchRate = 75.f;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Camera")
	float MouseSensitivity = 0.6f;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Camera")
	float DefaultFieldOfView = 88.f;

	UPROPERTY(EditAnywhere, Category = "CoinRush|Camera")
	float SprintFieldOfView = 99.f;

	float CameraYaw = 0.f;
	float CameraPitch = -20.f;
	float LandShakeTime = 0.f;
	float LandShakeDuration = 0.22f;
	bool bSprinting = false;
	bool bJumpKeyHeld = false;
	bool bEnterKeyHeld = false;
};

