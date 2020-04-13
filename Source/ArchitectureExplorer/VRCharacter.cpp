// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(LeftController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false); // Make sure teleport cylinder doesn't show at start

	if (BlinkerMaterialBase)
	{
		BlinkerDynamicMaterial = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, NULL);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerDynamicMaterial);
		BlinkerDynamicMaterial->SetScalarParameterValue(TEXT("Radius"), 0.4f);
	}

	// DynamicMesh = NewObject<UStaticMeshComponent>(this);
	// DynamicMesh->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	// DynamicMesh->SetStaticMesh(TeleportArcMesh);
	// DynamicMesh->SetMaterial(0, TeleportArcMaterial);
	// DynamicMesh->RegisterComponent();
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCharacterVRRootLocation();
	UpdateDestinationMarker();

	UpdateBlinker();
}

void AVRCharacter::UpdateBlinker()
{
	float PlayerSpeedMeters = GetVelocity().Size() / 100;
	// UE_LOG(LogTemp, Display, TEXT("Player speed %f"), PlayerSpeedMeters);

	float BlinkerRadius = RadiusVsVelocity->GetFloatValue(PlayerSpeedMeters);
	BlinkerDynamicMaterial->SetScalarParameterValue(TEXT("Radius"), BlinkerRadius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerDynamicMaterial->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}
	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}
	int32 SizeX;
	int32 SizeY;
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return FVector2D(0.5, 0.5);
	}
	PlayerController->GetViewportSize(OUT SizeX, OUT SizeY); // Get screen dimensions in pixels
	FVector2D ScreenLocation;
	PlayerController->ProjectWorldLocationToScreen(WorldStationaryLocation, OUT ScreenLocation);
	UE_LOG(LogTemp, Display, TEXT("Screen location: %s"), *ScreenLocation.ToString())
	// Normalize to U,V instead of pixels
	return FVector2D(ScreenLocation.X / SizeX, ScreenLocation.Y / SizeY);

	// else
	// {
	// 	return FVector2D(0.5, 0.5);
	// }
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector Location;
	TArray<FVector> PathArray;
	if (FindTeleportDestination(OUT Location, OUT PathArray))
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Location);
		DrawTeleportPath(PathArray);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
		TeleportPath->ClearSplinePoints();
	}
}

bool AVRCharacter::FindTeleportDestination(FVector& OutLocation, TArray<FVector>& OutPathArray)
{
	FPredictProjectilePathParams ParabolicParams = {
	    TeleportProjectileRadius,
	    LeftController->GetComponentLocation(),
	    LeftController->GetForwardVector() * TeleportProjectileSpeed,
	    5.f,
	    ECollisionChannel::ECC_Visibility,
	    this //ignore our own character as a target
	};
	ParabolicParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	ParabolicParams.bTraceComplex = true;
	FPredictProjectilePathResult ParabolicResult;

	bool bHit = UGameplayStatics::PredictProjectilePath(GetWorld(), ParabolicParams, OUT ParabolicResult);
	if (!bHit)
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("HitResult actor: %s"), *ParabolicResult.HitResult.Actor->GetName())
	FNavLocation OutNavLocation;

	// Project the hit result onto nav mesh plane
	bool bNav = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(ParabolicResult.HitResult.Location, OUT OutNavLocation, TeleportProjectionExtent);
	if (!bNav)
	{
		return false;
	}
	TArray<FVector> PathArray;

	// Extract from data the path spline
	for (auto& Point : ParabolicResult.PathData)
	{
		PathArray.Add(Point.Location);
	}
	OutPathArray = PathArray;

	OutLocation = OutNavLocation.Location;
	return true;
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector>& PathArray)
{
	TeleportPath->ClearSplinePoints(false);
	for (auto Point : PathArray)
	{
		UE_LOG(LogTemp, Display, TEXT("Spline location: %s"), *Point.ToString());
		TeleportPath->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
	}
	TeleportPath->UpdateSpline();

	// for (int i = 0; i < PathArray.Num(); i++)
	// {
	// 	if (TeleportPathMeshPool.Num() <= i)
	// 	{
	// 		UStaticMeshComponent* PoolMesh = NewObject<UStaticMeshComponent>(this);
	// 		PoolMesh->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	// 		PoolMesh->SetStaticMesh(TeleportArcMesh);
	// 		PoolMesh->SetMaterial(0, TeleportArcMaterial);
	// 		PoolMesh->RegisterComponent();
	// 		TeleportPathMeshPool.Add(PoolMesh);
	// 	}
	// 	TeleportPathMeshPool[i]->SetWorldLocation(PathArray[i]);
	// }

	// Hide all meshes
	for (auto PoolMesh : TeleportPathMeshPool)
	{
		PoolMesh->SetVisibleFlag(false);
	}

	// Instantiate new meshes as needed
	int32 PathSize = PathArray.Num();
	int32 PoolSize = TeleportPathMeshPool.Num();
	if (PoolSize < PathSize)
	{
		for (int i = 0; i < PathSize - PoolSize; i++)
		{
			UStaticMeshComponent* PoolMesh = NewObject<UStaticMeshComponent>(this);
			PoolMesh->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			PoolMesh->SetStaticMesh(TeleportArcMesh);
			PoolMesh->SetMaterial(0, TeleportArcMaterial);
			PoolMesh->RegisterComponent();
			TeleportPathMeshPool.Add(PoolMesh);
		}
	}

	// TeleportPathMeshPool.SetNum(PathArray.Num(), false);

	// Set mesh locations, only show used meshes
	for (int i = 0; i < PathArray.Num(); i++)
	{
		TeleportPathMeshPool[i]->SetWorldLocation(PathArray[i]);
		TeleportPathMeshPool[i]->SetVisibleFlag(true);
	}
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
	if (DestinationMarker->IsVisible())
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
