// Fill out your copyright notice in the Description page of Project Settings.


#include "TeleportPortal.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TP_FirstPerson/TP_FirstPersonCharacter.h"


// Sets default values
ATeleportPortal::ATeleportPortal()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(FName("Default Scene Root"));
	RootComponent = DefaultSceneRoot;

	Frame = CreateDefaultSubobject<UStaticMeshComponent>(FName("Frame"));
	Frame->SetupAttachment(RootComponent);

	PortalPlane = CreateDefaultSubobject<UStaticMeshComponent>(FName("Portal"));
	PortalPlane->SetupAttachment(RootComponent);

	ForwardDirection = CreateDefaultSubobject<UArrowComponent>(FName("ForwardDirection"));
	ForwardDirection->SetupAttachment(RootComponent);

	PortalCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(FName("PortalCamera"));
	PortalCamera->SetupAttachment(RootComponent);
	PortalCamera->CompositeMode = ESceneCaptureCompositeMode::SCCM_Composite;
	PortalCamera->bCaptureEveryFrame = false;
	PortalCamera->bCaptureOnMovement = false;
	PortalCamera->bAlwaysPersistRenderingState = true;

	Detection = CreateDefaultSubobject<UBoxComponent>(FName("Detection"));
	Detection->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATeleportPortal::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle DelayHandle;
	GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this]()
	{
		SetTickGroup(ETickingGroup::TG_PostUpdateWork);
		this->CreateDynamicMaterialInstance();
		if (LinkedPortal && Portal_RT)
		{
			LinkedPortal->PortalCamera->TextureTarget = Portal_RT;
		}
		SetClipPlanes();
	}, 0.1f, false);
}

// Called every frame
void ATeleportPortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSceneCaptureRecursive(FVector(), FRotator());
	UpdateViewportSize();
	CheckTeleportPlayer();
}

void ATeleportPortal::CreateDynamicMaterialInstance()
{
	if (MaterialParentToDynamic)
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(MaterialParentToDynamic, this);
		Portal_MAT = DynamicMaterial;
		PortalPlane->SetMaterial(0, Portal_MAT);

		if(GEngine)
		{
			if (ULocalPlayer* LocalPlayer = GEngine->GetFirstGamePlayer(GetWorld()))
			{
				if (LocalPlayer->ViewportClient)
				{
					auto size = LocalPlayer->ViewportClient->Viewport->GetSizeXY();
					Portal_RT = NewObject<UTextureRenderTarget2D>();
					Portal_RT->InitAutoFormat(size.X, size.Y);
					Portal_RT->UpdateResourceImmediate(true);

					Portal_RT->bAutoGenerateMips = false;
				}
			}
		}

		Portal_MAT->SetTextureParameterValue("Texture", Portal_RT);
		Portal_MAT->SetVectorParameterValue("OffsetDistance", ForwardDirection->GetForwardVector() * OffsetAmount);
	}
}

/** @Deprecated **/
void ATeleportPortal::UpdateSceneCapture()
{
	if (LinkedPortal)
	{
		//Update location

		FVector updatedLocation = this->UpdateSceneCapture_GetUpdatedSceneCaptureLocation(FVector());
		FRotator updatedRotation = this->UpdateSceneCapture_GetUpdatedSceneCaptureRotation(FRotator());


		LinkedPortal->PortalCamera->SetWorldLocationAndRotation(updatedLocation, updatedRotation);
	}
}

void ATeleportPortal::SetClipPlanes()
{
	if (LinkedPortal)
	{
		FVector ClipPlaneBase = PortalCamera->GetComponentTransform().GetLocation() + (ForwardDirection->
			GetForwardVector() * -3);


		PortalCamera->bEnableClipPlane = true;
		PortalCamera->ClipPlaneBase = ClipPlaneBase;
		PortalCamera->ClipPlaneNormal = ForwardDirection->GetForwardVector();
	}
}

void ATeleportPortal::UpdateViewportSize()
{
	if(GEngine)
	{
		if (ULocalPlayer* LocalPlayer = GEngine->GetFirstGamePlayer(GetWorld()))
		{
			if (LocalPlayer->ViewportClient && Portal_RT)
			{
				auto size = LocalPlayer->ViewportClient->Viewport->GetSizeXY();

				if (! (Portal_RT->SizeX == size.X && Portal_RT->SizeY == size.Y))
				{
					Portal_RT->ResizeTarget(size.X, size.Y);
				}
			}
		}
	}
}

void ATeleportPortal::CheckTeleportPlayer()
{
	TArray<AActor*> OverlappingActors;
	Detection->GetOverlappingActors(OverlappingActors);
	
	for (AActor* OverlappingActor : OverlappingActors)
	{
		ACharacter* OverlappingCharacter = Cast<ACharacter>(OverlappingActor);
		if (OverlappingCharacter && OverlappingCharacter->IsPlayerControlled())
		{
			// Obtém a câmera do personagem
			UCameraComponent* CameraComponent = OverlappingCharacter->FindComponentByClass<UCameraComponent>();
			if (CameraComponent && IsPlayerCrossingPortal(CameraComponent->GetComponentTransform().GetLocation()))
			{
				DoTeleportPlayer();
			}
		} else if(OverlappingActor != this)
		{
			DoTeleport(OverlappingActor);
		}
	}

}

void ATeleportPortal::DoTeleportPlayer()
{
	ACharacter* player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (player && LinkedPortal)
	{
		FVector Location = DoTeleportPlayer_GetNewLocation();
		FRotator Rotation = DoTeleportPlayer_GetNewRotation(player->GetActorRotation());
		FTransform T;
		T.SetLocation(Location);
		T.SetRotation(Rotation.Quaternion());
		T.SetScale3D(T.GetScale3D());

		player->SetActorTransform(T);
		FRotator newRotation = DoTeleportPlayer_GetNewRotation(player->GetController()->GetControlRotation());
		player->GetController()->SetControlRotation(newRotation);
		player->GetMovementComponent()->Velocity = UpdatePlayerVelocity(player->GetMovementComponent()->Velocity);

		if(ATP_FirstPersonCharacter* castedPlayer = Cast<ATP_FirstPersonCharacter>(player))
		{
			castedPlayer->SmoothOrientation(newRotation);
		}
		UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->SetGameCameraCutThisFrame();
	}
}

void ATeleportPortal::DoTeleport(AActor* actorToTeleport)
{
	if(actorToTeleport && GEngine)
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, actorToTeleport->GetName());
}

FVector ATeleportPortal::UpdatePlayerVelocity(FVector Velocity)
{
	FVector Dir = Velocity;
	if (LinkedPortal)
	{
		Dir.Normalize(0.0001);

		Dir = UKismetMathLibrary::InverseTransformDirection(GetActorTransform(), Dir);
		Dir = UKismetMathLibrary::MirrorVectorByNormal(Dir, FVector(1, 0, 0));
		Dir = UKismetMathLibrary::MirrorVectorByNormal(Dir, FVector(0, 1, 0));
		Dir = UKismetMathLibrary::TransformDirection(LinkedPortal->GetActorTransform(), Dir);
		return Dir * Velocity.Length();
	}
	return FVector();
}

FVector ATeleportPortal::DoTeleportPlayer_GetNewLocation()
{
	ACharacter* player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (player && LinkedPortal)
	{
		FTransform T = GetActorTransform();
		FVector Scale = T.GetScale3D();
		Scale.X *= -1;
		Scale.Y *= -1;
		T.SetScale3D(Scale);


		FVector Location = UKismetMathLibrary::InverseTransformLocation(T, player->GetActorLocation());
		Location = UKismetMathLibrary::TransformLocation(LinkedPortal->GetActorTransform(), Location);
		return Location;
	}

	return FVector();
}

FRotator ATeleportPortal::DoTeleportPlayer_GetNewRotation(FRotator rotation)
{
	if (LinkedPortal)
	{
		FTransform portalTransform = LinkedPortal->GetActorTransform();
		FVector X;
		FVector Y;
		FVector Z;
		UKismetMathLibrary::GetAxes(rotation, X, Y, Z);
		X = UKismetMathLibrary::InverseTransformDirection(GetActorTransform(), X);
		X = UKismetMathLibrary::MirrorVectorByNormal(X, FVector(1, 0, 0));
		X = UKismetMathLibrary::MirrorVectorByNormal(X, FVector(0, 1, 0));
		X = UKismetMathLibrary::TransformDirection(portalTransform, X);

		Y = UKismetMathLibrary::InverseTransformDirection(GetActorTransform(), Y);
		Y = UKismetMathLibrary::MirrorVectorByNormal(Y, FVector(1, 0, 0));
		Y = UKismetMathLibrary::MirrorVectorByNormal(Y, FVector(0, 1, 0));
		Y = UKismetMathLibrary::TransformDirection(portalTransform, Y);

		Z = UKismetMathLibrary::InverseTransformDirection(GetActorTransform(), Z);
		Z = UKismetMathLibrary::MirrorVectorByNormal(Z, FVector(1, 0, 0));
		Z = UKismetMathLibrary::MirrorVectorByNormal(Z, FVector(0, 1, 0));
		Z = UKismetMathLibrary::TransformDirection(portalTransform, Z);

		return UKismetMathLibrary::MakeRotationFromAxes(X, Y, Z);
	}

	return FRotator();
}

bool ATeleportPortal::IsPlayerCrossingPortal(FVector point)
{
	FVector PortalLocation = GetActorLocation();
	FVector PortalNormal = ForwardDirection->GetForwardVector();

	bool isInFront = PortalNormal.Dot(point - PortalLocation) >= 0;

	FPlane FPortalPlane = UKismetMathLibrary::MakePlaneFromPointAndNormal(PortalLocation, PortalNormal);
	float outT;
	FVector outIntersection;

	bool isIntersection = UKismetMathLibrary::LinePlaneIntersection(LastPosition, point, FPortalPlane, outT,
	                                                                outIntersection);

	bool isCrossing = (isIntersection && !isInFront && LastInFront);
	LastInFront = isInFront;
	LastPosition = point;

	return isCrossing;
}

FRotator ATeleportPortal::UpdateSceneCapture_GetUpdatedSceneCaptureRotation(FRotator OldRotation)
{
	if (!LinkedPortal)
	{
		return FRotator();
	}
	FTransform ActorTransform = GetActorTransform();


	FVector X;
	FVector Y;
	FVector Z;
	UKismetMathLibrary::BreakRotIntoAxes(OldRotation, X, Y, Z);


	X = UKismetMathLibrary::InverseTransformDirection(ActorTransform, X);
	X = UKismetMathLibrary::MirrorVectorByNormal(X, FVector(1, 0, 0));
	X = UKismetMathLibrary::MirrorVectorByNormal(X, FVector(0, 1, 0));
	FVector TransformDirectionForward = UKismetMathLibrary::TransformDirection(LinkedPortal->GetActorTransform(), X);

	Y = UKismetMathLibrary::InverseTransformDirection(ActorTransform, Y);
	Y = UKismetMathLibrary::MirrorVectorByNormal(Y, FVector(1, 0, 0));
	Y = UKismetMathLibrary::MirrorVectorByNormal(Y, FVector(0, 1, 0));
	FVector TransformDirectionRight = UKismetMathLibrary::TransformDirection(LinkedPortal->GetActorTransform(), Y);

	Z = UKismetMathLibrary::InverseTransformDirection(ActorTransform, Z);
	Z = UKismetMathLibrary::MirrorVectorByNormal(Z, FVector(1, 0, 0));
	Z = UKismetMathLibrary::MirrorVectorByNormal(Z, FVector(0, 1, 0));
	FVector TransformDirectionUp = UKismetMathLibrary::TransformDirection(LinkedPortal->GetActorTransform(), Z);


	return UKismetMathLibrary::MakeRotationFromAxes(TransformDirectionForward, TransformDirectionRight,
	                                                TransformDirectionUp);
}

void ATeleportPortal::UpdateSceneCaptureRecursive(FVector Location, FRotator Rotation)
{

	if(LinkedPortal)
	{
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		FTransform CameraManagerTransform = CameraManager->GetTransform();

		if(CurrentRecursion == 0) {
			
			FVector TemporaryLocation = this->UpdateSceneCapture_GetUpdatedSceneCaptureLocation(CameraManagerTransform.GetLocation());
			FRotator TemporaryRotation = this->UpdateSceneCapture_GetUpdatedSceneCaptureRotation(CameraManagerTransform.GetRotation().Rotator());
			CurrentRecursion++;
			UpdateSceneCaptureRecursive(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->SetWorldLocationAndRotation(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->CaptureScene();
			CurrentRecursion = 0;
		} else if(CurrentRecursion < MaxRecursion) {
			FVector TemporaryLocation = UpdateSceneCapture_GetUpdatedSceneCaptureLocation(Location);
			FRotator TemporaryRotation = UpdateSceneCapture_GetUpdatedSceneCaptureRotation(Rotation);
			CurrentRecursion++;
			UpdateSceneCaptureRecursive(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->SetWorldLocationAndRotation(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->CaptureScene();
		} else
		{
			FVector TemporaryLocation = UpdateSceneCapture_GetUpdatedSceneCaptureLocation(Location);
			FRotator TemporaryRotation = UpdateSceneCapture_GetUpdatedSceneCaptureRotation(Rotation);
			LinkedPortal->PortalCamera->SetWorldLocationAndRotation(TemporaryLocation, TemporaryRotation);
			PortalPlane->SetVisibility(false);
			LinkedPortal->PortalCamera->CaptureScene();
			PortalPlane->SetVisibility(true);
		}
	}
	
}

FVector ATeleportPortal::UpdateSceneCapture_GetUpdatedSceneCaptureLocation(FVector OldLocation)
{
	if (!LinkedPortal)
	{
		return FVector();
	}
	FTransform actorTransform = GetActorTransform();
	FVector scale = actorTransform.GetScale3D();

	scale.X *= -1;
	scale.Y *= -1;
	actorTransform.SetScale3D(scale);
	
	FVector inverseTransformLocation = UKismetMathLibrary::InverseTransformLocation(actorTransform, OldLocation);
	FTransform LinkedPortalTransform = LinkedPortal->GetActorTransform();

	FVector updatedLocation = UKismetMathLibrary::TransformLocation(LinkedPortalTransform, inverseTransformLocation);
	return updatedLocation;
}
