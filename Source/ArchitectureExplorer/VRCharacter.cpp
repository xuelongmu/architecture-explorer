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

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

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
		// BlinkerDynamicMaterial->SetScalarParameterValue(TEXT("Radius"), 0.4f);
		BlinkerDynamicMaterial->SetScalarParameterValue(TEXT("Radius"), 2.0f); //disabled blinker
	}

	if (HandControllerBP)
	{
		LeftController = GetWorld()->SpawnActor<AHandController>(HandControllerBP);
		if (LeftController)
		{
			LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			LeftController->SetHand(EControllerHand::Left);
			LeftController->SetOwner(this);
		}

		RightController = GetWorld()->SpawnActor<AHandController>(HandControllerBP);
		if (RightController)
		{
			RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			RightController->SetHand(EControllerHand::Right);
			RightController->SetOwner(this);
		}
		// Right controller pairing handled within this method.
		LeftController->PairController(RightController);
	}
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

	//disabled blinker
	// float BlinkerRadius = RadiusVsVelocity->GetFloatValue(PlayerSpeedMeters);
	// BlinkerDynamicMaterial->SetScalarParameterValue(TEXT("Radius"), BlinkerRadius);

	// FVector2D Center = GetBlinkerCenter();
	// BlinkerDynamicMaterial->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
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
	// UE_LOG(LogTemp, Display, TEXT("Screen location: %s"), *ScreenLocation.ToString())
	// Normalize to U,V instead of pixels
	return FVector2D(ScreenLocation.X / SizeX, ScreenLocation.Y / SizeY);
}

// Called to bind functionality to &AVR
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Move_Y"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Move_X"), this, &AVRCharacter::MoveRight);

	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Released, this, &AVRCharacter::ReleaseRight);
}

void AVRCharacter::MoveForward(float Throttle)
{
	if (FMath::Abs(Throttle) > 0.1)
	{
		AddMovementInput(Throttle * Camera->GetForwardVector());
	}
}

void AVRCharacter::MoveRight(float Throttle)
{
	if (FMath::Abs(Throttle) > 0.1)
	{
		AddMovementInput(Throttle * Camera->GetRightVector());
	}
}

void AVRCharacter::BeginTeleport()
{
	UE_LOG(LogTemp, Display, TEXT("Teleport requested to %s"), *DestinationMarker->GetComponentLocation().ToString());
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
	UE_LOG(LogTemp, Display, TEXT("Teleport performed to %s"), *DestinationLocation.ToString());

	StartFade(1, 0);
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
		HideTeleportPath();
	}
}

bool AVRCharacter::FindTeleportDestination(FVector& OutLocation, TArray<FVector>& OutPathArray)
{
	FPredictProjectilePathParams ParabolicParams = {
	    TeleportProjectileRadius,
	    LeftController->GetActorLocation(),
	    LeftController->GetActorForwardVector() * TeleportProjectileSpeed,
	    5.f,
	    ECollisionChannel::ECC_Visibility,
	    this //ignore our own character as a target
	};
	// ParabolicParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	ParabolicParams.bTraceComplex = true;
	FPredictProjectilePathResult ParabolicResult;

	bool bHit = UGameplayStatics::PredictProjectilePath(GetWorld(), ParabolicParams, OUT ParabolicResult);
	if (!bHit)
	{
		return false;
	}

	// UE_LOG(LogTemp, Display, TEXT("HitResult actor: %s"), *ParabolicResult.HitResult.Actor->GetName())
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

void AVRCharacter::HideTeleportPath()
{
	// Hide all meshes
	for (auto SplineMesh : TeleportPathMeshPool)
	{
		SplineMesh->SetVisibility(false);
	}
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector>& PathArray)
{
	TeleportPath->ClearSplinePoints(false);

	// Construct spline
	for (auto Point : PathArray)
	{
		// UE_LOG(LogTemp, Display, TEXT("Spline location: %s"), *Point.ToString());
		TeleportPath->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
	}

	TeleportPath->UpdateSpline();

	/* 
	// Udemy method
	for (int i = 0; i < PathArray.Num() - 1; i++)
	{
		if (TeleportPathMeshPool.Num() - 1 <= i)
		{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->RegisterComponent();
			SplineMesh->SetVisibility(false);
			TeleportPathMeshPool.Add(SplineMesh);
		}
		USplineMeshComponent* SplineMesh = TeleportPathMeshPool[i];

		FVector StartLocation, StartTangent, EndLocation, EndTangent;

		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, OUT StartLocation, OUT StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, OUT EndLocation, OUT EndTangent);
		SplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);
		SplineMesh->SetVisibility(true);
	} */

	HideTeleportPath();

	// Instantiate new meshes as needed
	int32 SegmentSize = PathArray.Num() - 1;
	int32 CurrentPoolSize = TeleportPathMeshPool.Num();
	if (CurrentPoolSize < SegmentSize)
	{
		for (int i = 0; i < SegmentSize - CurrentPoolSize; i++)
		{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);

			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->RegisterComponent();
			SplineMesh->SetVisibility(false);
			TeleportPathMeshPool.Add(SplineMesh);
		}
	}

	// Set mesh locations and tangents. Only show used meshes
	for (int i = 0; i < PathArray.Num() - 1; i++)
	{
		USplineMeshComponent* SplineMesh = TeleportPathMeshPool[i];

		FVector StartLocation, StartTangent, EndLocation, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, OUT StartLocation, OUT StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, OUT EndLocation, OUT EndTangent);

		SplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);

		SplineMesh->SetVisibility(true);
	}
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
	// VRRoot->SetRelativeLocation(NewCameraOffset);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (PlayerController)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, FadeInDuration, FLinearColor::Black);
	}
}
