// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	FindCapsuleComponent();
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCapsuleLocation();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}
void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::FindCapsuleComponent()
{
	Collider = FindComponentByClass<UCapsuleComponent>();
	if (!Collider)
	{
		UE_LOG(LogTemp, Error, TEXT("Capsule component not found for %s"), *GetName());
	}
}

void AVRCharacter::UpdateCapsuleLocation()
{
	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	UE_LOG(LogTemp, Display, TEXT("Camera movement: %s"), *NewCameraOffset.ToString());
	// Don't want the capsule to move vertically
	NewCameraOffset.Z = 0;

	if (!Collider)
	{
		UE_LOG(LogTemp, Error, TEXT("Collider pointer not found for %s"));

		return;
	}
	// Move collider to where the camera is
	AddActorWorldOffset(NewCameraOffset);
	// Collider->SetRelativeLocation(NewCameraOffset);

	// Invert the vector and apply to VR Root to prevent positive feedback loop
	FVector InvertCameraOffset = -NewCameraOffset;

	UE_LOG(LogTemp, Display, TEXT("VR Root Translation : %s"), *InvertCameraOffset.ToString());
	if (!VRRoot)
	{
		UE_LOG(LogTemp, Error, TEXT("VRRoot pointer not found for %s"));
		return;
	}
	VRRoot->AddWorldOffset(InvertCameraOffset);
	// VRRoot->SetRelativeLocation(InvertCameraOffset);
}

// void AVRCharacter::UpdateVRRootLocation(FVector TranslationToApply)
// {
// 	// VRRoot->SetRelativeLocation(TranslationToApply);
// }