// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "TeleportPortal.generated.h"

UCLASS()
class PUZZLE_API ATeleportPortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATeleportPortal();
	UPROPERTY(EditAnywhere)
	USceneComponent* DefaultSceneRoot;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Frame;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PortalPlane;
	UPROPERTY(EditAnywhere)
	float OffsetAmount = -4.f;


	
	UPROPERTY(EditAnywhere)
	UArrowComponent* ForwardDirection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneCaptureComponent2D* PortalCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* Detection;


	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* MaterialParentToDynamic;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstanceDynamic* Portal_MAT;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* Portal_RT;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ATeleportPortal* LinkedPortal;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector LastPosition;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool LastInFront;

	UPROPERTY(EditAnywhere)
	int MaxRecursion = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsActivated = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float clipPlanesFactor = -1.0;
	
	UPROPERTY(VisibleAnywhere)
	int CurrentRecursion;

	UPROPERTY(VisibleAnywhere)
	FGuid UniqueID;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	bool IsActorVisibleByCamera();

private:

	UFUNCTION(BlueprintCallable)
	void CreateDynamicMaterialInstance();

	UFUNCTION(BlueprintCallable)
	void UpdateSceneCapture();

	UFUNCTION(BlueprintCallable)
	void SetClipPlanes();

	UFUNCTION(BlueprintCallable)
	void PreventCameraClipping();

	UFUNCTION(BlueprintCallable)
	void UpdateViewportSize();
	
	UFUNCTION(BlueprintCallable)
	void CheckTeleportPlayer();

	UFUNCTION(BlueprintCallable)
	void HandleCharacterTeleport(class ACharacter* OverlappingCharacter, FName TeleportedTag, FName LinkedTeleportedTag);

	UFUNCTION(BlueprintCallable)
	void HandleActorTeleport(AActor* OverlappingActor, FName TeleportedTag, FName LinkedTeleportedTag);

	UFUNCTION(BlueprintCallable)
	void PerformTeleport(AActor* Actor, FName TeleportedTag, FName LinkedTeleportedTag);
	
	UFUNCTION(BlueprintCallable)
	void AddTeleportTagsWithTimer(AActor* Actor, FName TeleportedTag, FName LinkedTeleportedTag);

	UFUNCTION(BlueprintCallable)
	bool IsMovingTowardsPortal(const FVector& Velocity);

	UFUNCTION(BlueprintCallable)
	bool IsPlayerCrossingPortal(FVector point);

	UFUNCTION(BlueprintCallable)
	void DoTeleportPlayer();

	UFUNCTION(BlueprintCallable)
	void CallSmoothRotation(ACharacter* player, FRotator Rotation);

	UFUNCTION(BlueprintCallable)
	void DoTeleport(AActor* actorToTeleport);

	UFUNCTION(BlueprintCallable)
	FVector UpdateActorVelocity(FVector Velocity);
	
	UFUNCTION(BlueprintCallable)
	FRotator UpdateActorRotation(FRotator Rotation);


	FVector DoTeleport_GetActorNewLocation(AActor* actor);
	FRotator DoTeleport_GetActorNewRotation(FRotator rotation);
	
	

	FVector UpdateSceneCapture_GetUpdatedSceneCaptureLocation(FVector OldLocation);
	FRotator UpdateSceneCapture_GetUpdatedSceneCaptureRotation(FRotator OldRotation);

	UFUNCTION(BlueprintCallable)
	void UpdateSceneCaptureRecursive(FVector Location, FRotator Rotation);

	UFUNCTION()
	void RemoveTeleportTag(AActor* actorToRemoveTag);
	
};
