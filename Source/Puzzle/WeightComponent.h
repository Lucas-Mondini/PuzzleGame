// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HaveWeight.h"
#include "GameFramework/Actor.h"
#include "WeightComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PUZZLE_API UWeightComponent : public UActorComponent, public IHaveWeight
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UWeightComponent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weight", meta=(AllowPrivateAccess="true"))
	float Weight = 10.0f;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weight")
	int GetWeight();
	virtual int GetWeight_Implementation() override;
};
