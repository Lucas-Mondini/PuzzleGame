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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UWeightComponent* WeightComponent;
	
};
