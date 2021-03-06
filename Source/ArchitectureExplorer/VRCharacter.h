// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HandController.h"
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
	// Configuration Parameters
	UPROPERTY(VisibleAnywhere)
	AHandController* LeftController;
	UPROPERTY(VisibleAnywhere)
	AHandController* RightController;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;
	
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY()
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;
	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 800;
	UPROPERTY(EditAnywhere)
	float TeleportSimulationTime = 1;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 1.0f;
	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;


	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly)
	class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* TeleportArchMaterial;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AHandController> HandControllerClass;
	
private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	void GripLeft() { LeftController->Grip(); }
	void ReleaseLeft() { LeftController->Release(); }
	void GripRight() { RightController->Grip(); }
	void ReleaseRight() { RightController->Release(); }

	void BeginTeleport();
	void Teleport();
	void UpdateDestinationMarker();
	bool FindTeleportDestination(TArray<FVector> &OutPath, FVector& OutLocation);
	void DrawTeleportPath(TArray<FVector>& Path);
	void UpdateSpline(const TArray<FVector>& Path);
	void UpdateBlinkers();

	FVector2D GetBlinkersCentre();

	bool IsValidNavMeshHit(FHitResult &OutHit, FVector& OutLocation);

	
};
