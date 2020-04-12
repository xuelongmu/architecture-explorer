// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"

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
	void UpdateCharacterVRRootLocation();
	bool FindTeleportDestination(FVector& OutLocation);
	void UpdateDestinationMarker();
	// void UpdateVRRootLocation(FVector TranslationToApply);
	void StartFade(float FromAlpha, float ToAlpha);

	void MoveForward(float throttle);
	void MoveRight(float throttle);
	void BeginTeleport();
	void FinishTeleport();
	void UpdateBlinker();
	FVector2D GetBlinkerCenter();

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	UMotionControllerComponent* LeftController;

	UPROPERTY(VisibleAnywhere)
	UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere)
	UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY()
	UMaterialInstanceDynamic* BlinkerDynamicMaterial;

	UPROPERTY(EditAnywhere)
	UCurveFloat* RadiusVsVelocity;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* DestinationMarker;

	UPROPERTY(EditAnywhere)
	float MaxTeleportDistance = 2000.f;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100.f, 100.f, 100.f);

	UPROPERTY(EditAnywhere)
	float FadeInDuration = 0.5f;

	UPROPERTY(EditAnywhere)
	float FadeOutDuration = 0.5f;
};
