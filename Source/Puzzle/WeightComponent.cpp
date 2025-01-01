// Fill out your copyright notice in the Description page of Project Settings.


#include "WeightComponent.h"

// Sets default values
UWeightComponent::UWeightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void UWeightComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

int UWeightComponent::GetWeight_Implementation()
{
	return Weight;
}


