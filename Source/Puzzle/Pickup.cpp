// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"

#include "Character/ALSCharacter.h"
#include "Components/BoxComponent.h"

// Sets default values
APickup::APickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(DefaultSceneRoot);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickupMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// PickupBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupBox"));
	// PickupBox->SetupAttachment(DefaultSceneRoot);
	// PickupBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// PickupBox->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void APickup::Interact_Implementation(AActor* Interactor)
{
	if(AALSCharacter* als = Cast<AALSCharacter>(Interactor))
	{
		als->Pickup(this);
	}
}

// Called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

