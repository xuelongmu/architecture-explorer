// Fill out your copyright notice in the Description page of Project Settings.

#include "HandController.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define OUT

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
	UpdateClimb();
}

void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// UE_LOG(LogTemp, Display, TEXT("Begin OverlappedActor: %s"), *OverlappedActor->GetName());

	if (!bCanClimb && CanClimb())
	{
		bCanClimb = true;
		UE_LOG(LogTemp, Warning, TEXT("Can Climb"));
		PlayHandHoldRumble();
	}
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// UE_LOG(LogTemp, Display, TEXT("End OverlappedActor: %s"), *OverlappedActor->GetName());

	if (bCanClimb && !CanClimb())
	{
		bCanClimb = false;
		UE_LOG(LogTemp, Warning, TEXT("Cannot Climb"));
	}
}
bool AHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OUT OverlappingActors);
	for (auto Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag(TEXT("Climbable")))
		{
			// UE_LOG(LogTemp, Warning, TEXT("Climbable actor found: %s"), *Actor->GetName());

			return true;
		}
	}
	// UE_LOG(LogTemp, Warning, TEXT("No climbable actors found"));

	return false;
}

void AHandController::PlayHandHoldRumble()
{
	if (bIsClimbing) // Avoid constant rumbling weirdness when player is climbing
	{
		return;
	}

	EControllerHand Hand = MotionController->GetTrackingSource();

	APlayerController* PlayerController = nullptr;

	if (GetCharacterPlayerController(OUT PlayerController))
	{
		FString HandString = UEnum::GetValueAsString(Hand);

		UE_LOG(LogTemp, Warning, TEXT("Player controller name: %s"), *PlayerController->GetName());

		PlayerController->PlayHapticEffect(HandHoldRumble, Hand); // If not working, try restarting SteamVR or replacing batteries.
	}
}

bool AHandController::GetCharacterPlayerController(APlayerController*& OutPlayerController)
{
	auto Pawn = Cast<APawn>(GetAttachParentActor());

	if (!Pawn)
	{
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());

	if (!PlayerController)
	{
		return false;
	}

	OutPlayerController = PlayerController;
	UE_LOG(LogTemp, Warning, TEXT("Player controller name: %s"), *PlayerController->GetName());
	// UE_LOG(LogTemp, Warning, TEXT("Out Player controller name: %s"), *OutPlayerController->GetName());

	return true;
}

void AHandController::Grip()
{
	// Only update StartLocation if the character is not currently climbing
	if (bCanClimb && !bIsClimbing)
	{
		bIsClimbing = true;
		ClimbingStartLocation = GetActorLocation();
		auto Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character)
		{
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		}
		OtherController->Release();
	}
}

void AHandController::Release()
{
	if (bIsClimbing)
	{
		bIsClimbing = false;
		// If both controllers are not climbing, then make the character fall down
		if (!OtherController->bIsClimbing)
		{
			auto Character = Cast<ACharacter>(GetAttachParentActor());
			if (Character)
			{
				Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
			}
		}
	}
}

void AHandController::UpdateClimb()
{
	if (!bIsClimbing)
	{
		return;
	}
	FVector HandControllerDelta = GetActorLocation() - ClimbingStartLocation;

	GetAttachParentActor()->AddActorWorldOffset(-HandControllerDelta);
}
