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

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

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
		BlinkerMaterialInstance->SetScalarParameterValue(TEXT("BlinkerSize"), 0.25f);
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
	FHitResult OutHit;
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + MaxTeleportDistance * Camera->GetForwardVector();
	//DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false);
	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility);
	if (OutHit.IsValidBlockingHit() && IsValidNavMeshHit(OutHit))
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(OutHit.Location);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
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
