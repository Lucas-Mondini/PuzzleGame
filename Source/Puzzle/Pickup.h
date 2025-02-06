// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactable.h"
#include "GameFramework/Actor.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Pickup.generated.h"

UENUM(BlueprintType, meta = (ScriptName = "PickupType"))
enum class PickupType : uint8
{
	Default,
	Box = EALSOverlayState::Box,
	Barrel = EALSOverlayState::Barrel
};

UCLASS()
class PUZZLE_API APickup : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	class UBoxComponent* BoxCollision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	class UStaticMeshComponent* PickupMesh;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	// class UBoxComponent* PickupBox;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor);

	// use a ALSOverlayState enum to determine if the pickup will be held as a box or as a barrel
	// ENSURE that it only has two states, Barrel and Box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	PickupType PickupType = PickupType::Box;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	FVector Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	FTransform InitialTransform;
	
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
