// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzleCharacter.h"

#include "Interactable.h"
#include "Pickup.h"
#include "WeightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
		//add player velocity to the item
		//pickupAsHeld->PickupMesh->AddImpulse(GetVelocity() * 1000, NAME_None, true);
		
	} else if (ActorToInteract)
	{
		if (ActorToInteract->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		{
			IInteractable::Execute_Interact(ActorToInteract, this);
		}
	}
}

void APuzzleCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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

	bool hittedAnInteractable = false;
	if (GetWorld()->LineTraceMultiByObjectType(InteractHits, TraceStart, TraceEnd, Traces, QueryParams))
	{
		for (const FHitResult& InteractHit : InteractHits)
		{
			if (InteractHit.GetActor() != this && InteractHit.GetActor()->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
			{
				if (ActorToInteract != InteractHit.GetActor())
				{
					if (ActorToInteract)
					{
						IInteractable::Execute_StopLooking(ActorToInteract);
					}
					ActorToInteract = InteractHit.GetActor();
					IInteractable::Execute_Looked(ActorToInteract);
				}
				hittedAnInteractable = true;
				break;
			}
		}
	}

	if(!hittedAnInteractable && ActorToInteract)
	{
		IInteractable::Execute_StopLooking(ActorToInteract);
		ActorToInteract = nullptr;
	}
}

