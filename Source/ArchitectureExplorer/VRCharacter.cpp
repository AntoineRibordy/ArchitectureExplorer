// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);
	
	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	DestinationMarker->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);

	if (BlinkerMaterialBase != nullptr)
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
		BlinkerMaterialInstance->SetScalarParameterValue(TEXT("BlinkersSize"), 0.25f);
	}

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector CameraOffsetLocation = Camera->GetComponentLocation() - GetActorLocation();
	CameraOffsetLocation.Z = 0;
	AddActorWorldOffset(CameraOffsetLocation);
	VRRoot->AddWorldOffset(-CameraOffsetLocation);
	UpdateDestinationMarker();
	UpdateBlinkers();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float Value)
{
	AddMovementInput(Camera->GetForwardVector(), Value);
}

void AVRCharacter::MoveRight(float Value)
{
	AddMovementInput(Camera->GetRightVector(), Value);
}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr) return;
	float Speed = GetVelocity().Size();
	// We convert the speed in meters per second so divide by 100
	float Radius = RadiusVsVelocity->GetFloatValue(Speed / 100);
	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("BlinkersSize"), Radius);

	FVector2D Centre = GetBlinkersCentre();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Centre"), FLinearColor(Centre.X,  Centre.Y, 0));

	
}

FVector2D AVRCharacter::GetBlinkersCentre()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}
	// If going backwards, substract movement direction from camera location
	bool IsMovingForward = FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) >= 0 ? true : false;
	FVector WorldStationaryLocation;
	if (IsMovingForward)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	FVector2D ScreenStationaryLocation;
	PlayerController->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);
	int32 ViewportSizeX, ViewportSizeY;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	ScreenStationaryLocation.X /= ViewportSizeX;
	ScreenStationaryLocation.Y /= ViewportSizeY;
	return ScreenStationaryLocation;
}

void AVRCharacter::BeginTeleport()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController != nullptr) 
	{
		// Fade camera out
		PlayerController->PlayerCameraManager->StartCameraFade(0, 1, TeleportFadeTime, FLinearColor::Black);
	}
	// Wait for fade to finish before teleporting
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AVRCharacter::Teleport, TeleportFadeTime);
}

void AVRCharacter::Teleport()
{
	VRRoot->SetWorldLocation(DestinationMarker->GetComponentLocation() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * FVector::UpVector);
}

void AVRCharacter::UpdateDestinationMarker()
{
	//DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false);
	//GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility);
	TArray<FVector> Path;
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Path, Location);
	if (bHasDestination)//Result.HitResult.IsValidBlockingHit())
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Location);
		UpdateSpline(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

bool AVRCharacter::FindTeleportDestination(TArray<FVector> &OutPath, FVector& OutLocation)
{
	if (RightController == nullptr) return false;
	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();
	//Look = Look.RotateAngleAxis(30, RightController->GetRightVector());
	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start,
		TeleportProjectileSpeed * Look,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility);
	FPredictProjectilePathResult Result;
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true;
	bool bHit = UGameplayStatics::PredictProjectilePath(RightController, Params, Result);

	if (!bHit) return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData)
	{
		OutPath.Add(PointData.Location);
	}

	return IsValidNavMeshHit(Result.HitResult);

}

void AVRCharacter::UpdateSpline(const TArray<FVector>& Path)
{
	TeleportPath->ClearSplinePoints(false);

	for (int Index = 0; Index <Path.Num(); ++Index)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[Index]);
		TeleportPath->AddPoint(FSplinePoint(Index, LocalPosition, ESplinePointType::Curve), false);
	}

	TeleportPath->UpdateSpline();
}

bool AVRCharacter::IsValidNavMeshHit(FHitResult &OutHit)
{
	UNavigationSystemV1* NavMesh = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	FNavLocation OutLocation;
	if (NavMesh != nullptr)
	{
		// Is the point we're looking at a valid navigation point on the navmesh
		return NavMesh->ProjectPointToNavigation(OutHit.Location, OutLocation, TeleportProjectionExtent);
	}
	else
	{
		// If we don't have a nav mesh, we can't judge the validity of the hit, so return true
		return true;
	}
}
