#include "GhostReplayer.h"

#include "GhostComponent.h"
#include "PuzzleCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Character/ALSPlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

AGhostReplayer::AGhostReplayer()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	RootComponent = Mesh;

	Detection = CreateDefaultSubobject<UBoxComponent>(TEXT("Detection"));
	Detection->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Detection->SetCollisionResponseToAllChannels(ECR_Overlap);
	Detection->SetupAttachment(Mesh);

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetupAttachment(Detection);
	
}

void AGhostReplayer::BeginPlay()
{
	Super::BeginPlay();
}


void AGhostReplayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector BoxExtent = Detection->GetScaledBoxExtent();
	FVector BoxLocation = Detection->GetComponentLocation();

	TArray<AActor*> FullyOverlappingActors;

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor)
		{
			continue;
		}

		FBox ActorBounds = Actor->GetComponentsBoundingBox();
		FVector ActorMin = ActorBounds.Min;
		FVector ActorMax = ActorBounds.Max;

		// Verifique se o ator está completamente dentro da área
		if (ActorMin.X >= BoxLocation.X - BoxExtent.X &&
			ActorMax.X <= BoxLocation.X + BoxExtent.X &&
			ActorMin.Y >= BoxLocation.Y - BoxExtent.Y &&
			ActorMax.Y <= BoxLocation.Y + BoxExtent.Y &&
			ActorMin.Z >= BoxLocation.Z - BoxExtent.Z &&
			ActorMax.Z <= BoxLocation.Z + BoxExtent.Z)
		{
			FullyOverlappingActors.Add(Actor);
		}
	}

	for (AActor* FullyOverlappingActor : FullyOverlappingActors)
	{
		if (!TrackedActors.Contains(FullyOverlappingActor))
		{
			if (FullyOverlappingActor->IsA(ACharacter::StaticClass()))
			{
				APuzzleCharacter* Character = Cast<APuzzleCharacter>(FullyOverlappingActor);
				if (Character)
				{
					Character->CameraComponent->PostProcessSettings.WeightedBlendables.Array.Empty();
					Character->CameraComponent->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PostProcessMaterial));
					// Obtém o PlayerController associado ao Character
					APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
					if (PlayerController)
					{
						// Obtém o índice do PlayerController no mundo
						int32 PlayerIndex = PlayerController->GetLocalPlayer()->GetControllerId();

						// Obtém o CameraManager correto usando o índice do PlayerController
						APlayerCameraManager* Manager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), PlayerIndex);
						if (Manager)
						{
							// Cast para AALSPlayerCameraManager
							if (AALSPlayerCameraManager* ALSManager = Cast<AALSPlayerCameraManager>(Manager))
							{
								// Limpa e adiciona o material no PostProcess
								ALSManager->bUpdatePostProcessSettings = true;
							}
						}
					}
				}
			}

			
			// Adicione o ator à lista de rastreados
			TrackedActors.Add(FullyOverlappingActor, FullyOverlappingActor->GetActorTransform());
			SpawnGhost(FullyOverlappingActor);

			// Configure o timer específico para este ator
			FTimerHandle& ActorTimer = ActorTimers.FindOrAdd(FullyOverlappingActor);
			GetWorld()->GetTimerManager().SetTimer(ActorTimer, [this, FullyOverlappingActor]()
			{
				StartReplay(FullyOverlappingActor);
				RestartActorPosition(FullyOverlappingActor);
			}, captureTime, true);
		}
	}
}

void AGhostReplayer::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	while (OtherActor && OtherActor->GetParentActor() != nullptr)
	{
		OtherActor = OtherActor->GetParentActor();
	}
	FTimerHandle DelayHandle;
	GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this, OtherActor]()
	{
		//must be a Character
		if (!OtherActor || IsGhostActor(OtherActor) || TrackedActors.Contains(OtherActor) || OtherActor->Tags.Contains("Untrackable") || !OtherActor->IsA(ACharacter::StaticClass()))
		{
			return;
		}

		// Adicione à lista de potencialmente rastreados
		OverlappingActors.AddUnique(OtherActor);	
	}, 0.1f, false);
	
}


void AGhostReplayer::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// Certifique-se de pegar o ator raiz
	while (OtherActor->GetParentActor() != nullptr)
	{
		OtherActor = OtherActor->GetParentActor();
	}

	// Pare de rastrear o ator
	if (OverlappingActors.Contains(OtherActor))
	{
		OverlappingActors.Remove(OtherActor);
	}

	if (TrackedActors.Contains(OtherActor))
	{
		StopTrackingActor(OtherActor);
	}
}


void AGhostReplayer::StopTrackingActor(AActor* Actor)
{
	if (!Actor) return;

	// Remova da lista de atores sobrepostos
	OverlappingActors.Remove(Actor);
	

	// Remova da lista de atores rastreados
	if (TrackedActors.Contains(Actor))
	{
		if (Actor->IsA(ACharacter::StaticClass()))
		{
			APuzzleCharacter* Character = Cast<APuzzleCharacter>(Actor);
			if (Character)
			{
				Character->CameraComponent->PostProcessSettings.WeightedBlendables.Array.Empty();
				int PlayerID = Character->GetPlayerState()->GetPlayerId();
				APlayerCameraManager* manager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), PlayerID);
				if (manager)
				{
					//cast to AALSPlayerCameraManager
					if(AALSPlayerCameraManager* alsManager = Cast<AALSPlayerCameraManager>(manager))
					{
						alsManager->bUpdatePostProcessSettings = true;
					}
					
				}
				
			}
		}
		// Limpe e remova o timer associado ao ator
		if (ActorTimers.Contains(Actor))
		{
			GetWorld()->GetTimerManager().ClearTimer(ActorTimers[Actor]);
			ActorTimers.Remove(Actor);
		}

		// Destrua todos os Ghosts associados ao ator
		DestroyGhostsForActor(Actor);

		// Remova da lista de rastreados
		TrackedActors.Remove(Actor);
	}
}


void AGhostReplayer::StartReplay(AActor* Actor)
{
	if (TrackedActors.Contains(Actor))
	{
		// Reproduza os ghosts deste ator
		if (GhostActors.Contains(Actor))
		{
			int i = 0;
			for (AActor* Ghost : GhostActors[Actor].Ghosts)
			{
				if (Ghost)
				{
					UGhostComponent* GhostComponent = Ghost->FindComponentByClass<UGhostComponent>();
					if (GhostComponent)
					{
						GhostComponent->StartReplay();
					}
				} else
				{
					//remove ghost from array using index
					GhostActors[Actor].Ghosts.RemoveAt(i);
				}
				i++;
			}
		}
	}
}

void AGhostReplayer::RestartActorPosition(AActor* Actor)
{
	if (TrackedActors.Contains(Actor))
	{
		Actor->SetActorTransform(TrackedActors[Actor]);
		SpawnGhost(Actor);
	}
}


void AGhostReplayer::SpawnGhost(AActor* Actor)
{
	if (TrackedActors.Contains(Actor))
	{
		UWorld* World = GetWorld();
		if (World)
		{

			AActor* Ghost = World->SpawnActor<AActor>(Actor->GetClass(), Actor->GetActorTransform());
			if (Ghost)
			{
				UGhostComponent* GhostComponent = NewObject<UGhostComponent>(Ghost);
				if(GhostComponent)
				{
					GhostComponent->RegisterComponent();
					Ghost->AddOwnedComponent(GhostComponent);

					GhostComponent->captureInterval = captureInterval;
					GhostComponent->captureTime = captureTime;
					GhostComponent->ReplayCount = DefaultReplayCount;
					GhostComponent->FollowTarget = Actor;
					GhostComponent->CollisionResponses = CollisionResponses;
					GhostComponent->TargetIconActorClass = TargetIconActorClass;
					GhostComponent->ParentGhostReplayer = this;
					GhostComponent->SpawnTimer();

					if (GhostMaterial)
					{
						TArray<UMeshComponent*> MeshComps;
						Ghost->GetComponents<UMeshComponent>(MeshComps);

						for (UMeshComponent* MeshComp : MeshComps)
						{
							if (!MeshComp) continue;
							const int32 NumMats = MeshComp->GetNumMaterials();
							for (int32 i = 0; i < NumMats; i++)
							{
								MeshComp->SetMaterial(i, GhostMaterial);
							}
						}
					}

					if(GhostActors.Contains(Actor))
					{
						GhostActors[Actor].Ghosts.Add(Ghost);
					} else {
						FGhostsArray GhostsArray;
						GhostsArray.Ghosts.Add(Ghost);
						GhostActors.Add(Actor, GhostsArray);
					}
					
				} else
				{
					Ghost->Destroy();
				}
			}
		}
	}
}


void AGhostReplayer::DestroyGhostsForActor(AActor* Actor)
{
	if (GhostActors.Contains(Actor))
	{
		TArray<AActor*> Ghosts =  GhostActors[Actor].Ghosts;
		
		//FActorData& Data = TrackedActors[Actor];
		for (AActor* Ghost : Ghosts)
		{
			if (Ghost)
			{
				Ghost->Destroy();
			}
		}
		GhostActors[Actor].Ghosts.Empty();
		GhostActors.Remove(Actor);
	}
}


bool AGhostReplayer::IsGhostActor(AActor* Actor) const
{
	if(!Actor)
	{
		return false;
	}

	UGhostComponent* GhostComponent = Actor->FindComponentByClass<UGhostComponent>();
	if (GhostComponent)
	{
		return true;
	}
	return false;
}

void AGhostReplayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (auto& Timer : ActorTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer.Value);
	}

	ActorTimers.Empty();
}