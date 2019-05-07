// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* LeftController;
	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;
	
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
	float MaxTeleportDistance = 2000;
	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 1.0f;
	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;
	
	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;
	
	void MoveForward(float Value);
	void MoveRight(float Value);

	void BeginTeleport();
	void Teleport();
	void UpdateDestinationMarker();
	void UpdateBlinkers();

	FVector2D GetBlinkersCentre();

	bool IsValidNavMeshHit(FHitResult &OutHit);
};
