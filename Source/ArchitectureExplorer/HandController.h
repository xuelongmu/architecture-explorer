// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionControllerComponent.h"

#include "HandController.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AHandController : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHandController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void SetHand(EControllerHand Hand) { MotionController->SetTrackingSource(Hand); }
	void PairController(AHandController* Controller);
	void Grip();
	void Release();

private:
	// callbacks
	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	// helpers
	bool CanClimb() const;

	void PlayHandHoldRumble();

	bool GetCharacterPlayerController(APlayerController** OutPlayerController);
	// APlayerController* GetCharacterPlayerController() const;

	void UpdateClimb();

	// default subobject
	UPROPERTY(VisibleAnywhere)
	UMotionControllerComponent* MotionController;

	UPROPERTY(EditDefaultsOnly)
	UHapticFeedbackEffect_Base* HandHoldRumble;

	UPROPERTY(EditDefaultsOnly)
	UForceFeedbackEffect* HandHoldForceFeedback;

	// state
	UPROPERTY(VisibleAnywhere)
	bool bCanClimb = false;

	UPROPERTY(VisibleAnywhere)
	bool bIsClimbing = false;

	UPROPERTY(VisibleAnywhere)
	FVector ClimbingStartLocation;

	// Able to edit from the other class?
	UPROPERTY(VisibleAnywhere)
	AHandController* OtherController;
};
