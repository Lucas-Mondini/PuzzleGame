// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#include "Character/ALSCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "AI/ALSAIController.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Puzzle/Pickup.h"
#include "Puzzle/WeightComponent.h"

AALSCharacter::AALSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeldObjectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HeldObjectRoot"));
	HeldObjectRoot->SetupAttachment(GetMesh());

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(HeldObjectRoot);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(HeldObjectRoot);

	DropLocation = CreateDefaultSubobject<USceneComponent>(TEXT("DropLocation"));
	DropLocation->SetupAttachment(RootComponent);

	AIControllerClass = AALSAIController::StaticClass();
}

void AALSCharacter::Pickup_Implementation(APickup* Item)
{
	if(Item)
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

void AALSCharacter::Internal_InteractAction()
{
	//if is holding an object, drop it
	if(HeldObject)
	{
		HeldObject->SetActorHiddenInGame(false);
		HeldObject->SetActorEnableCollision(true);
		HeldObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		HeldObject->SetActorRotation(FRotator::ZeroRotator);
		//set left hand socket location - offset as it's location
		
		//HeldObject->SetActorLocation(GetMesh()->GetSocketLocation(TEXT("spine_02")) + FVector(0, 100, 0) * GetActorForwardVector() + FVector(0, 0, 50));
		HeldObject->PickupMesh->SetSimulatePhysics(true);
		HeldObject->SetActorLocation(DropLocation->GetComponentLocation(), false, nullptr, ETeleportType::None);
		HeldObject = nullptr;
		SetOverlayState(EALSOverlayState::Default);
		this->ClearHeldObject();
		
	} else
	{
		Super::Internal_InteractAction();
	}
}

void AALSCharacter::ClearHeldObject()
{
	StaticMesh->SetStaticMesh(nullptr);
	SkeletalMesh->SetSkeletalMesh(nullptr);
	SkeletalMesh->SetAnimInstanceClass(nullptr);
}

void AALSCharacter::AttachToHand(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh, UClass* NewAnimClass,
                                 bool bLeftHand, FVector Offset)
{
	ClearHeldObject();

	if (IsValid(NewStaticMesh))
	{
		StaticMesh->SetStaticMesh(NewStaticMesh);
	}
	else if (IsValid(NewSkeletalMesh))
	{
		SkeletalMesh->SetSkeletalMesh(NewSkeletalMesh);
		if (IsValid(NewAnimClass))
		{
			SkeletalMesh->SetAnimInstanceClass(NewAnimClass);
		}
	}

	FName AttachBone;
	if (bLeftHand)
	{
		AttachBone = TEXT("VB LHS_ik_hand_gun");
	}
	else
	{
		AttachBone = TEXT("VB RHS_ik_hand_gun");
	}

	HeldObjectRoot->AttachToComponent(GetMesh(),
	                                  FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBone);
	HeldObjectRoot->SetRelativeLocation(Offset);
}

void AALSCharacter::RagdollStart()
{
	ClearHeldObject();
	Super::RagdollStart();
}

void AALSCharacter::RagdollEnd()
{
	Super::RagdollEnd();
	UpdateHeldObject();
}

ECollisionChannel AALSCharacter::GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius)
{
	const FName CameraSocketName = bRightShoulder ? TEXT("TP_CameraTrace_R") : TEXT("TP_CameraTrace_L");
	TraceOrigin = GetMesh()->GetSocketLocation(CameraSocketName);
	TraceRadius = 15.0f;
	return ECC_Camera;
}

FTransform AALSCharacter::GetThirdPersonPivotTarget()
{
	return FTransform(GetActorRotation(),
	                  (GetMesh()->GetSocketLocation(TEXT("Head")) + GetMesh()->GetSocketLocation(TEXT("root"))) / 2.0f,
	                  FVector::OneVector);
}

FVector AALSCharacter::GetFirstPersonCameraTarget()
{
	return GetMesh()->GetSocketLocation(TEXT("FP_Camera"));
}

void AALSCharacter::OnOverlayStateChanged(EALSOverlayState PreviousState)
{
	Super::OnOverlayStateChanged(PreviousState);
	UpdateHeldObject();
}

void AALSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHeldObjectAnimations();
}

void AALSCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHeldObject();
}
