// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HandController.h"
#include "Materials/MaterialInstanceDynamic.h"

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

private: //methods
	void UpdateCharacterVRRootLocation();
	void StartFade(float FromAlpha, float ToAlpha);
	void MoveForward(float Throttle);
	void MoveRight(float Throttle);
	void GripLeft() { LeftController->Grip(); }
	void ReleaseLeft() { LeftController->Release(); }
	void GripRight() { RightController->Grip(); }
	void ReleaseRight() { RightController->Release(); }
	void BeginTeleport();
	void FinishTeleport();
	bool FindTeleportDestination(FVector& OutLocation, TArray<FVector>& PathArray);
	void DrawTeleportPath(const TArray<FVector>& PathArray);
	void HideTeleportPath();
	void UpdateDestinationMarker();
	void UpdateBlinker();
	FVector2D GetBlinkerCenter();

private: //state objects
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	AHandController* LeftController;

	UPROPERTY(VisibleAnywhere)
	AHandController* RightController;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	TArray<USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(VisibleAnywhere)
	UPostProcessComponent* PostProcessComponent;

	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* BlinkerDynamicMaterial;

private: // configuration parameters
	UPROPERTY(EditAnywhere)
	UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AHandController> HandControllerBP;

	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* TeleportArcMesh;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* TeleportArcMaterial;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 5.f;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 1000.f;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileTime = 5.f;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100.f, 100.f, 100.f);

	UPROPERTY(EditAnywhere)
	UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditAnywhere)
	float FadeInDuration = 0.5f;

	UPROPERTY(EditAnywhere)
	float FadeOutDuration = 0.5f;
};
