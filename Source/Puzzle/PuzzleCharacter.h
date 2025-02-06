// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacter.h"
#include "PuzzleCharacter.generated.h"

/**
 * 
 */
UCLASS()
class PUZZLE_API APuzzleCharacter : public AALSCharacter
{
	GENERATED_BODY()

public:
	APuzzleCharacter(const FObjectInitializer& ObjectInitializer);


	virtual void Pickup_Implementation(AActor* Item) override;

	virtual void Internal_InteractAction() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadWrite)
	FRotator InitialControllerRotation;

	UPROPERTY(BlueprintReadWrite)
	FRotator InitialWorldRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UWeightComponent* WeightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interactable")
	AActor* ActorToInteract;

	UFUNCTION(BlueprintImplementableEvent)
	void SmoothOrientation(FRotator newControlRotation);
	
};
