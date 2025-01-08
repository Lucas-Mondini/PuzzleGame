// Fill out your copyright notice in the Description page of Project Settings.


#include "TeleportPortal.h"

#include "Camera/CameraComponent.h"
#include "Character/ALSCharacter.h"
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
	PortalCamera->bAlwaysPersistRenderingState = false;

	Detection = CreateDefaultSubobject<UBoxComponent>(FName("Detection"));
	Detection->SetupAttachment(RootComponent);

	UniqueID = FGuid::NewGuid();
	this->Tags.Add("Unteleportable");
}

// Called when the game starts or when spawned
void ATeleportPortal::BeginPlay()
{
	Super::BeginPlay();

	GLog->Log("BeginPlay");

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

	if(bIsActivated)
	{
		if(IsActorVisibleByCamera())
		{
			UpdateSceneCaptureRecursive(FVector(), FRotator());
		}
		PreventCameraClipping();
		UpdateViewportSize();
		CheckTeleportPlayer();
		SetClipPlanes();
	} else
	{
		if(PortalPlane->IsVisible())
		{
			PortalPlane->SetVisibility(false);
		}
	}
}

bool ATeleportPortal::IsActorVisibleByCamera()
{
    // PlayerController e checagens básicas
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager)
    {
        return false;
    }

    // Pega a localização da câmera
    const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();

    // Bounding box do portal
    const FBox BoundingBox = GetComponentsBoundingBox();
    const FVector Min = BoundingBox.Min;
    const FVector Max = BoundingBox.Max;

    // Cantos do bounding box
    TArray<FVector> Corners;
    Corners.Add(FVector(Min.X, Min.Y, Min.Z));
    Corners.Add(FVector(Min.X, Min.Y, Max.Z));
    Corners.Add(FVector(Min.X, Max.Y, Min.Z));
    Corners.Add(FVector(Min.X, Max.Y, Max.Z));
    Corners.Add(FVector(Max.X, Min.Y, Min.Z));
    Corners.Add(FVector(Max.X, Min.Y, Max.Z));
    Corners.Add(FVector(Max.X, Max.Y, Min.Z));
    Corners.Add(FVector(Max.X, Max.Y, Max.Z));

    // Tamanho do viewport (largura e altura em pixels)
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PC->GetViewportSize(ViewportX, ViewportY);

    // Verifica cada canto
    for (const FVector& Corner : Corners)
    {
        // 1) Projecta o canto para as coordenadas de tela
        FVector2D ScreenPos;
        bool bOnScreen = UGameplayStatics::ProjectWorldToScreen(PC, Corner, ScreenPos);

        // Se a projeção falhou, ou está fora do viewport, não conta
        if (!bOnScreen)
        {
            continue;
        }
        if (ScreenPos.X < 0.f || ScreenPos.X > ViewportX ||
            ScreenPos.Y < 0.f || ScreenPos.Y > ViewportY)
        {
            continue;
        }

        // 2) Verifica se o canto está dentro da distância máxima de render
        const float DistToCorner = FVector::Dist(CameraLocation, Corner);
        if (DistToCorner <= MaxRenderDistance)
        {
            // Se pelo menos um canto está “na tela” e dentro do range,
            // consideramos o portal como visível
            return true;
        }
    }

    // Se nenhum canto passou nos critérios, o portal não está visível/rendereizável
    return false;
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
	// if (LinkedPortal)
	// {
	// 	//Update location
	//
	// 	FVector updatedLocation = this->UpdateSceneCapture_GetUpdatedSceneCaptureLocation(FVector());
	// 	FRotator updatedRotation = this->UpdateSceneCapture_GetUpdatedSceneCaptureRotation(FRotator());
	//
	//
	// 	LinkedPortal->PortalCamera->SetWorldLocationAndRotation(updatedLocation, updatedRotation);
	// }
}

void ATeleportPortal::SetClipPlanes()
{
	if (LinkedPortal)
	{
		FVector ClipPlaneBase = PortalPlane->GetComponentTransform().GetLocation() +
			ForwardDirection->GetForwardVector() * clipPlanesFactor;
		
		PortalCamera->bEnableClipPlane = true;
		PortalCamera->ClipPlaneBase = ClipPlaneBase;
		PortalCamera->ClipPlaneNormal = ForwardDirection->GetForwardVector();		
	}
}

void ATeleportPortal::PreventCameraClipping()
{
	APlayerCameraManager* CameraManager =  UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	FVector CameraLocation = CameraManager->GetCameraLocation();
	
	
	double dot = UKismetMathLibrary::Dot_VectorVector(CameraLocation - GetActorLocation(), ForwardDirection->GetForwardVector());
	
	if (dot < 200 && dot > -5) {
		PortalPlane->SetRelativeLocation(
			FVector(UKismetMathLibrary::Clamp((1 - dot/50) * -5, -5, 0), 0, 146.0)
			);
	} else {
		PortalPlane->SetRelativeLocation(FVector(0, 0, 146.0));
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
	if (!LinkedPortal)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	Detection->GetOverlappingActors(OverlappingActors);

	FName TeleportedTag = FName("teleported" + UniqueID.ToString());
	FName LinkedTeleportedTag = FName("teleported" + LinkedPortal->UniqueID.ToString());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (ACharacter* OverlappingCharacter = Cast<ACharacter>(OverlappingActor))
		{
			HandleCharacterTeleport(OverlappingCharacter, TeleportedTag, LinkedTeleportedTag);
		}
		else
		{
			HandleActorTeleport(OverlappingActor, TeleportedTag, LinkedTeleportedTag);
		}
	}

}

void ATeleportPortal::HandleCharacterTeleport(ACharacter* OverlappingCharacter, FName TeleportedTag,
	FName LinkedTeleportedTag)
{
	if (!OverlappingCharacter || !OverlappingCharacter->IsPlayerControlled())
	{
		return;
	}

	if (OverlappingCharacter->Tags.Contains(TeleportedTag) ||
		OverlappingCharacter->Tags.Contains(LinkedTeleportedTag) ||
		OverlappingCharacter->Tags.Contains("Unteleportable"))
	{
		return;
	}

	if (AALSCharacter* CastedCharacter = Cast<AALSCharacter>(OverlappingCharacter))
	{
		FVector CharacterLocation = CastedCharacter->GetTransform().GetLocation();
		FVector Velocity = CastedCharacter->GetVelocity();
		if (IsPlayerCrossingPortal(CharacterLocation) && IsMovingTowardsPortal(Velocity))
		{
			PerformTeleport(CastedCharacter, TeleportedTag, LinkedTeleportedTag);
		}
	}
}

void ATeleportPortal::HandleActorTeleport(AActor* OverlappingActor, FName TeleportedTag, FName LinkedTeleportedTag)
{
	if (!OverlappingActor ||
		OverlappingActor == this ||
		OverlappingActor->Tags.Contains(TeleportedTag) ||
		OverlappingActor->Tags.Contains(LinkedTeleportedTag) ||
		OverlappingActor->Tags.Contains("Unteleportable"))
	{
		return;
	}

	FVector Velocity = OverlappingActor->GetVelocity();
	if(IsMovingTowardsPortal(Velocity))
	{
		DoTeleport(OverlappingActor);
		AddTeleportTagsWithTimer(OverlappingActor, TeleportedTag, LinkedTeleportedTag);
	}
}

void ATeleportPortal::PerformTeleport(AActor* Actor, FName TeleportedTag, FName LinkedTeleportedTag)
{
	DoTeleportPlayer();
	AddTeleportTagsWithTimer(Actor, TeleportedTag, LinkedTeleportedTag);
}

void ATeleportPortal::AddTeleportTagsWithTimer(AActor* Actor, FName TeleportedTag, FName LinkedTeleportedTag)
{
	if (!Actor)
	{
		return;
	}

	Actor->Tags.Add(TeleportedTag);
	Actor->Tags.Add(LinkedTeleportedTag);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([Actor, TeleportedTag, LinkedTeleportedTag]()
	{
		Actor->Tags.Remove(TeleportedTag);
		Actor->Tags.Remove(LinkedTeleportedTag);
	}), 0.1f, false);
}

bool ATeleportPortal::IsMovingTowardsPortal(const FVector& Velocity)
{
	FVector ForwardVector = ForwardDirection->GetForwardVector();
	return FVector::DotProduct(Velocity, ForwardVector) < 0.0f; // Negativo significa movimento contra o ForwardVector
}

void ATeleportPortal::DoTeleportPlayer()
{
	ACharacter* player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (player && LinkedPortal)
	{
		const FVector Location = DoTeleport_GetActorNewLocation(Cast<AActor>(player));
		const FRotator Rotation = DoTeleport_GetActorNewRotation(player->GetActorRotation());
		FTransform T;
		T.SetLocation(Location);
		T.SetRotation(Rotation.Quaternion());
		T.SetScale3D(T.GetScale3D());

		player->SetActorTransform(T);
		const FRotator NewRotation = DoTeleport_GetActorNewRotation(player->GetController()->GetControlRotation());
		player->GetController()->SetControlRotation(NewRotation);
		player->SetActorRotation(UpdateActorRotation(player->GetActorRotation()));
		player->GetMovementComponent()->Velocity = UpdateActorVelocity(player->GetMovementComponent()->Velocity);

		CallSmoothRotation(player, NewRotation);
		
		UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->SetGameCameraCutThisFrame();
	}
}

void ATeleportPortal::CallSmoothRotation(ACharacter* player, FRotator Rotation)
{
	FName FunctionName = TEXT("SmoothOrientation");
	UFunction* Function = player->FindFunction(FunctionName);
        
	if (Function)
	{
		// Prepara os parâmetros para a função
		struct
		{
			FRotator NewRotation;
		} Params;
		Params.NewRotation = Rotation;
            
		// Chama a função SmoothOrientation
		player->ProcessEvent(Function, &Params);
	}
}

void ATeleportPortal::DoTeleport(AActor* actorToTeleport)
{
	if (LinkedPortal)
	{
		const FVector Location = DoTeleport_GetActorNewLocation(actorToTeleport);
		GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, actorToTeleport->GetName() + " -> Teleported to " + Location.ToString());
		const FRotator Rotation = DoTeleport_GetActorNewRotation(actorToTeleport->GetActorRotation());
		FTransform T;
		T.SetLocation(Location);
		T.SetRotation(Rotation.Quaternion());
		T.SetScale3D(T.GetScale3D());
	
		actorToTeleport->SetActorTransform(T);
		actorToTeleport->GetRootComponent()->ComponentVelocity =  UpdateActorVelocity(actorToTeleport->GetVelocity());
		actorToTeleport->SetActorRotation(UpdateActorRotation(actorToTeleport->GetActorRotation()));
		// player->GetMovementComponent()->Velocity = UpdatePlayerVelocity(player->GetMovementComponent()->Velocity);
		
		UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->SetGameCameraCutThisFrame();
	}
}

FVector ATeleportPortal::UpdateActorVelocity(FVector Velocity)
{
	if (!LinkedPortal || Velocity.IsZero())
	{
		return FVector();
	}

	// Obter os vetores de direção do portal de entrada
	FVector ForwardVector = ForwardDirection->GetForwardVector();
	FVector RightVector = ForwardDirection->GetRightVector();
	FVector UpVector = ForwardDirection->GetUpVector();

	// Projeção da velocidade nos eixos do portal de entrada
	float ForwardSpeed = FVector::DotProduct(Velocity, ForwardVector);
	float RightSpeed = FVector::DotProduct(Velocity, RightVector);
	float UpSpeed = FVector::DotProduct(Velocity, UpVector);

	// Inverter ou ajustar componentes conforme necessário
	ForwardSpeed = -ForwardSpeed; // Inverter direção para frente, se necessário
	RightSpeed = -RightSpeed;     // Manter direção lateral
	UpSpeed = UpSpeed;           // Manter direção para cima

	// Reconstruir a velocidade no espaço do portal de saída
	FVector NewVelocity = LinkedPortal->ForwardDirection->GetForwardVector() * ForwardSpeed +
						  LinkedPortal->ForwardDirection->GetRightVector() * RightSpeed +
						  LinkedPortal->ForwardDirection->GetUpVector() * UpSpeed;

	return NewVelocity;
}

FRotator ATeleportPortal::UpdateActorRotation(FRotator Rotation)
{
	if (!LinkedPortal)
	{
		return FRotator();
	}

	// Obter a rotação do portal de entrada e saída
	FRotator PortalInRotation = ForwardDirection->GetComponentRotation();
	FRotator PortalOutRotation = LinkedPortal->ForwardDirection->GetComponentRotation();

	// Calcular o delta entre a rotação do ator e a do portal de entrada
	FRotator RelativeRotation = Rotation - PortalInRotation;

	// Ajustar para o espaço do portal de saída
	FRotator NewRotation = PortalOutRotation + RelativeRotation;

	// Inverter o yaw para apontar contra o portal de saída
	//NewRotation.Yaw += 180.0f;
	NewRotation.Normalize(); // Normalizar a rotação para evitar ângulos fora do intervalo válido

	return NewRotation;
}

FVector ATeleportPortal::DoTeleport_GetActorNewLocation(AActor* actor)
{
	if (actor && LinkedPortal)
	{
		FTransform T = GetActorTransform();
		FVector Scale = T.GetScale3D();
		Scale.X *= -1;
		Scale.Y *= -1;
		T.SetScale3D(Scale);


		FVector Location = UKismetMathLibrary::InverseTransformLocation(T, actor->GetActorLocation());
		Location = UKismetMathLibrary::TransformLocation(LinkedPortal->GetActorTransform(), Location);
		return Location;
	}

	return FVector();
}

FRotator ATeleportPortal::DoTeleport_GetActorNewRotation(FRotator rotation)
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

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if(LinkedPortal)
	{
		if(CurrentRecursion == 0) {
			// PortalCamera->HiddenComponents.Add(Frame);
			FVector TemporaryLocation = this->UpdateSceneCapture_GetUpdatedSceneCaptureLocation(CameraManager->GetCameraLocation());
			FRotator TemporaryRotation = this->UpdateSceneCapture_GetUpdatedSceneCaptureRotation(CameraManager->GetCameraRotation());
			CurrentRecursion++;
			
			UpdateSceneCaptureRecursive(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->SetWorldLocationAndRotation(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->CaptureScene();
			CurrentRecursion = 0;
		} else if(CurrentRecursion < MaxRecursion) {
			//PortalCamera->HiddenComponents.Remove(Frame);
			FVector TemporaryLocation = UpdateSceneCapture_GetUpdatedSceneCaptureLocation(Location);
			FRotator TemporaryRotation = UpdateSceneCapture_GetUpdatedSceneCaptureRotation(Rotation);
			CurrentRecursion++;
			UpdateSceneCaptureRecursive(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->SetWorldLocationAndRotation(TemporaryLocation, TemporaryRotation);
			LinkedPortal->PortalCamera->CaptureScene();
		} else
		{
			FVector TemporaryLocation = UpdateSceneCapture_GetUpdatedSceneCaptureLocation(Location);
			FRotator TemporaryRotation = Rotation;
			LinkedPortal->PortalCamera->SetWorldLocationAndRotation(TemporaryLocation, TemporaryRotation);
			PortalPlane->SetVisibility(false);
			LinkedPortal->PortalCamera->CaptureScene();
			PortalPlane->SetVisibility(true);
		}
	}
	
}

void ATeleportPortal::RemoveTeleportTag(AActor* actorToRemoveTag)
{
		if (actorToRemoveTag)
		{
			actorToRemoveTag->Tags.Remove("teleported");
		}
}

FVector ATeleportPortal::UpdateSceneCapture_GetUpdatedSceneCaptureLocation(FVector PlayerCameraLocation)
{
	if (!LinkedPortal)
	{
		return FVector();
	}
	FTransform actorTransform = GetActorTransform();
	FVector scale = actorTransform.GetScale3D();

	scale.X = -scale.X;
	scale.Y = -scale.Y;
	actorTransform.SetScale3D(scale);
	
	
	FTransform LinkedPortalTransform = LinkedPortal->GetActorTransform();
	FVector inverseTransformLocation = UKismetMathLibrary::InverseTransformLocation(actorTransform, PlayerCameraLocation);

	FVector updatedLocation = UKismetMathLibrary::TransformLocation(LinkedPortalTransform, inverseTransformLocation);
	return updatedLocation;
}
