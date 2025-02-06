// Fill out your copyright notice in the Description page of Project Settings.


#include "FollowerTimer.h"

#include "Components/TextRenderComponent.h"

// Sets default values
AFollowerTimer::AFollowerTimer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	Text->SetupAttachment(Mesh);

}

// Called when the game starts or when spawned
void AFollowerTimer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFollowerTimer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFollowerTimer::UpdatePercentage_Implementation(float InPercentage)
{
}


