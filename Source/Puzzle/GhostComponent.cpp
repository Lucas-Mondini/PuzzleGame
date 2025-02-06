// Fill out your copyright notice in the Description page of Project Settings.


#include "GhostComponent.h"

#include "FollowerTimer.h"
#include "GhostReplayer.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UGhostComponent::UGhostComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetTickGroup(ETickingGroup::TG_LastDemotable);

	// ...
}


// Called when the game starts
void UGhostComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorHiddenInGame(true);
		Owner->SetActorEnableCollision(false);

		// Configure a colisão para "Query Only"
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (Primitive)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Primitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			Primitive->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		}

		// Renomeie o ator para indicar que é um Ghost
		//Owner->SetActorLabel("Ghost" + Owner->GetActorLabel());

		//spawn a static mesh to follow the target
		// TargetIconActor = NewObject<UStaticMeshComponent>(FollowTarget);
		// TargetIconActor->SetStaticMesh(TargetIconMesh);
		// TargetIconActor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// TargetIconActor->SetCollisionResponseToAllChannels(ECR_Ignore);
		// TargetIconActor->SetupAttachment(FollowTarget->GetRootComponent());
		// TargetIconActor->RegisterComponent();
		
		
		
	}
}


// Called every frame
void UGhostComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(TargetIconActor && FollowTarget)
	{
		TargetIconActor->SetActorLocation(FollowTarget->GetActorLocation() + FVector(0, 0, 150));
		TargetIconActor->SetActorRotation(FollowTarget->GetActorRotation());

		if(AFollowerTimer* Timer = Cast<AFollowerTimer>(TargetIconActor))
		{
			GEngine->AddOnScreenDebugMessage(0, 0.1f, FColor::Green, FString::Printf(TEXT("Elapsed Time: %f"), ElapsedTime));
			GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Green, FString::Printf(TEXT("capture Time: %f"), captureTime));
			GEngine->AddOnScreenDebugMessage(2, 0.1f, FColor::Red, FString::Printf(TEXT("Percentage Calculated:: %f"), ElapsedTime / captureTime));
			const float percentage = ElapsedTime / captureTime;
			Timer->UpdatePercentage(percentage);
		}
	}

	if(ReplayCounter > ReplayCount)
	{
		GetOwner()->Destroy();
		return;
	}
	if(bIsCapturing)
	{
		ElapsedTime += DeltaTime;
		if(TickAccumulator >= captureInterval)
		{
			Capture();
			TickAccumulator = 0.0f;
		}
		else
		{
			TickAccumulator += DeltaTime;
		}
	} else if(bIsPlaying)
	{		
		iterateTransform();
	}
}


void UGhostComponent::iterateTransform()
{

	// Garante que temos um Owner que seja Character
	ACharacter* GhostChar = Cast<ACharacter>(GetOwner());
	if (!GhostChar)
	{
		bIsPlaying = false;
		return;
	}

	// Se ainda temos "frames" pra reproduzir
	if (CurrentTransformIndex < MovementSnapshots.Num())
	{
		const FMovementSnapshot& Snapshot = MovementSnapshots[CurrentTransformIndex];

		// 1) Força a posição e rotação
		GhostChar->SetActorLocation(Snapshot.Location);
		GhostChar->SetActorRotation(Snapshot.Rotation);

		// 2) Define a velocity do MovementComponent
		UCharacterMovementComponent* MoveComp = GhostChar->GetCharacterMovement();
		if (MoveComp)
		{
			MoveComp->Velocity = Snapshot.Velocity;

			// 3) Ajusta MovementMode
			MoveComp->SetMovementMode(EMovementMode(Snapshot.MovementMode));

			// Opcional: se quiser forçar "Falling" ou "Walking"
			// se preferir, confie apenas em MovementMode
			if (Snapshot.bIsFalling)
			{
				MoveComp->SetMovementMode(MOVE_Falling);
			}
			else
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}

			// Se quiser, força crouch/uncrouch de acordo com o snapshot
			if (Snapshot.bIsCrouched)
			{
				GhostChar->Crouch();
			}
			else
			{
				GhostChar->UnCrouch();
			}
		}

		// Avança pro próximo "frame"
		CurrentTransformIndex++;
	}
	else
	{
		// Fim do replay
		bIsPlaying = false;
	}
}


void UGhostComponent::Capture()
{
	if (ACharacter* Char = Cast<ACharacter>(FollowTarget))
	{
		// Cria um "frame" novo
		FMovementSnapshot Snapshot;

		Snapshot.Location     = Char->GetActorLocation();
		Snapshot.Rotation     = Char->GetActorRotation();
		Snapshot.Velocity     = Char->GetCharacterMovement()->Velocity;
		Snapshot.bIsFalling   = Char->GetCharacterMovement()->IsFalling();
		Snapshot.bIsCrouched  = Char->GetCharacterMovement()->IsCrouching(); 
		Snapshot.MovementMode = Char->GetCharacterMovement()->MovementMode;
		Snapshot.TimeStamp    = ElapsedTime; // só se precisar

		// Armazena no array
		MovementSnapshots.Add(Snapshot);
	}
	else
	{
		// Se não for Character, pode simplesmente capturar transforms...
		TransformArray.Add(FollowTarget->GetActorTransform());
	}
}

void UGhostComponent::SpawnTimer()
{
	UWorld* World = GetWorld();
	if (World && FollowTarget)
	{
		TargetIconActor = World->SpawnActor<AActor>(TargetIconActorClass, FollowTarget->GetActorLocation(), FollowTarget->GetActorRotation());

		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);

		TArray<AActor*> ActorsWithGhostComponent;
		for (AActor* Actor : AllActors)
		{
			if (Actor && Actor->FindComponentByClass<UGhostComponent>())
			{
				ActorsWithGhostComponent.Add(Actor);
			}
		}

		int countSameParent = 0;
		for (AActor* Actor : ActorsWithGhostComponent)
		{
			if (UGhostComponent *c = Actor->FindComponentByClass<UGhostComponent>())
			{
				if(c->ParentGhostReplayer == ParentGhostReplayer)
					countSameParent++;
			}
		}

		if(Cast<AFollowerTimer>(TargetIconActor) && ParentGhostReplayer)
		{
			AFollowerTimer* Timer = Cast<AFollowerTimer>(TargetIconActor);
			int totalGhostsLeft = this->ParentGhostReplayer->DefaultReplayCount - countSameParent + 1;
			Timer->Text->SetText(FText::FromString(FString::Printf(TEXT("%d"), totalGhostsLeft)));
			
		}
	}
}

void UGhostComponent::StartReplay()
{
	bIsCapturing = false;
	bIsPlaying = true;
	CurrentTransformIndex = 0;
	ReplayCounter++;

	
	
	if(TargetIconActor)
	{
		
		TargetIconActor->Destroy();
	}

	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorHiddenInGame(false);
		Owner->SetActorEnableCollision(true);
		Owner->SetActorEnableCollision(ECollisionEnabled::QueryOnly);

		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (Primitive)
		{
			Primitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			Primitive->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignorar a câmera
			Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
			Primitive->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			if (CollisionResponses.Num() > 0)
			{
				for (const auto& Pair : CollisionResponses)
				{
					Primitive->SetCollisionResponseToChannel(Pair.Key, Pair.Value);
				}	
			}
		}
	}
}

void UGhostComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if(TargetIconActor)
	{
		
		TargetIconActor->Destroy();
	}
}
