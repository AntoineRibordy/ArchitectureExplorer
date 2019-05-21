// Fill out your copyright notice in the Description page of Project Settings.

#include "HandController.h"
#include "VRCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values
AHandController::AHandController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);

}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);

}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsClimbing)
	{
		FVector HandControllerDelta = GetActorLocation() - ClimbingStartLocation;
		GetAttachParentActor()->AddActorWorldOffset(-HandControllerDelta);
	}
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb();
	if (!bCanClimb && bNewCanClimb)
	{
		APawn* Pawn = Cast<APawn>(GetAttachParentActor());
		if (Pawn != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
			if (PlayerController != nullptr)
			{
				EControllerHand Hand = MotionController->GetTrackingSource();
				PlayerController->PlayHapticEffect(HapticEffect, Hand);
			}
		}
	}
	bCanClimb = bNewCanClimb;
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb();
}

bool AHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag(TEXT("Climbable")))
		{
			return true;
		}
	}
	return false;
}

void AHandController::Grip()
{
	if (bCanClimb)
	{
		if (!bIsClimbing)
		{
			bIsClimbing = true;
			OtherController->bIsClimbing = false;
			ClimbingStartLocation = GetActorLocation();

			ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
			if (Character != nullptr)
			{
				Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
			}
		}
	}
	
}

void AHandController::Release()
{
	if (bIsClimbing)
	{
		bIsClimbing = false;
		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != nullptr)
		{
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}
		
}

void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
}
