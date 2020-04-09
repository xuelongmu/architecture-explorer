// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

#define OUT

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
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false); // Make sure teleport cylinder doesn't show at start
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCharacterVRRootLocation();
	UpdateDestinationMarker();
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector Location;
	if (FindTeleportDestination(OUT Location))
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Location);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

bool AVRCharacter::FindTeleportDestination(FVector& OutLocation)
{
	// GetActorLocation(),
	// GetActorLocation() + Camera->GetComponentRotation().Vector() * MaxTeleportDistance, // Calculate player reach
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * MaxTeleportDistance;
	FHitResult Hit;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
	    OUT Hit,
	    Start,
	    End,
	    ECC_Visibility);
	if (!bHit)
	{
		return false;
	}
	FNavLocation OutNavLocation;
	bool bNav = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Hit.Location, OUT OutNavLocation, TeleportProjectionExtent);
	if (!bNav)
	{
		return false;
	}
	OutLocation = OutNavLocation.Location;
	return true;
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::BeginTeleport()
{
	// only teleport if marker is at a valid location
	if (DestinationMarker->bVisible)
	{
		StartFade(0, 1);

		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, FadeInDuration);
	}
}

void AVRCharacter::FinishTeleport()
{
	FVector DestinationLocation = DestinationMarker->GetComponentLocation();
	DestinationLocation.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); // Avoid teleporting player into ground
	SetActorLocation(DestinationLocation);

	StartFade(1, 0);
}

void AVRCharacter::UpdateCharacterVRRootLocation()
{
	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();

	// Don't want the capsule to move vertically
	NewCameraOffset.Z = 0;

	AddActorWorldOffset(NewCameraOffset);

	// Invert the vector and apply to VR Root to prevent positive feedback loop
	FVector InvertCameraOffset = -NewCameraOffset;

	// UE_LOG(LogTemp, Display, TEXT("VR Root Translation : %s"), *InvertCameraOffset.ToString());
	if (!VRRoot)
	{
		UE_LOG(LogTemp, Error, TEXT("VRRoot pointer not found for %s"));
		return;
	}
	VRRoot->AddWorldOffset(InvertCameraOffset);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (PlayerController)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, FadeInDuration, FLinearColor::Black);
	}
}
