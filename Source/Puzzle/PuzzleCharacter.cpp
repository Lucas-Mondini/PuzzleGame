// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzleCharacter.h"

#include "Interactable.h"
#include "Pickup.h"
#include "WeightComponent.h"
#include "Components/SphereComponent.h"

APuzzleCharacter::APuzzleCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WeightComponent = CreateDefaultSubobject<UWeightComponent>(TEXT("WeightComponent"));
}


void APuzzleCharacter::Pickup_Implementation(AActor* I)
{
	if(APickup* Item = Cast<APickup>(I))
	{
		this->HeldObject = Item;
		SetOverlayState((EALSOverlayState)Item->PickupType);

		// Attach item to the character's hand
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
		Item->AttachToComponent(GetMesh(), AttachmentRules); // Anexa ao socket "hand_r"

		// Apply offset (if needed)
		Item->SetActorRelativeLocation(Item->Offset);
		AttachToHand(Item->PickupMesh->GetStaticMesh(), nullptr, nullptr, true, FVector::ZeroVector);

		// Hide the item and disable collision
		Item->SetActorHiddenInGame(true);
		Item->SetActorEnableCollision(false);
		Item->PickupMesh->SetSimulatePhysics(false);
	}
}

void APuzzleCharacter::Internal_InteractAction()
{
	Super::Internal_InteractAction();

	if(APickup* pickupAsHeld = Cast<APickup>(HeldObject))
	{
		pickupAsHeld->SetActorHiddenInGame(false);
		pickupAsHeld->SetActorEnableCollision(true);
		pickupAsHeld->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		pickupAsHeld->SetActorRotation(FRotator::ZeroRotator);
		//set left hand socket location - offset as it's location
		
		//pickupAsHeld->SetActorLocation(GetMesh()->GetSocketLocation(TEXT("spine_02")) + FVector(0, 100, 0) * GetActorForwardVector() + FVector(0, 0, 50));
		pickupAsHeld->PickupMesh->SetSimulatePhysics(true);
		pickupAsHeld->SetActorLocation(DropLocation->GetComponentLocation(), false, nullptr, ETeleportType::None);
		HeldObject = nullptr;
		SetOverlayState(EALSOverlayState::Default);
		this->ClearHeldObject();
		
	} else
	{
		FVector TraceStart;
		FVector TraceEnd;

		FVector PlayerEyesLoc;
		FRotator PlayerEyesRot;

		GetActorEyesViewPoint(PlayerEyesLoc, PlayerEyesRot);

		TraceStart = PlayerEyesLoc;
		TraceEnd = PlayerEyesLoc + PlayerEyesRot.Vector() * ActionRadiusSphere->GetScaledSphereRadius();

		FCollisionObjectQueryParams Traces;
		Traces.AddObjectTypesToQuery(ECC_WorldDynamic);
		Traces.AddObjectTypesToQuery(ECC_Pawn);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this); // Ignorar o próprio ator

		TArray<FHitResult> InteractHits;

		if (GetWorld()->LineTraceMultiByObjectType(InteractHits, TraceStart, TraceEnd, Traces, QueryParams))
		{
			for (const FHitResult& InteractHit : InteractHits)
			{
				if (InteractHit.GetActor() != this && InteractHit.GetActor()->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
				{
					// Call the interact function
					DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 1, 0, 1);	
					IInteractable::Execute_Interact(InteractHit.GetActor(), this);
				}
			}
		}
		else
		{
			DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1, 0, 1);
		}	
	}
}