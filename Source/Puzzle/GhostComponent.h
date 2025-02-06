// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostComponent.generated.h"

class UPoseableMeshComponent;

USTRUCT(BlueprintType)
struct FMovementSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	UPROPERTY(BlueprintReadWrite)
	FRotator Rotation;

	// Velocity que o MovementComponent estava usando naquele frame
	UPROPERTY(BlueprintReadWrite)
	FVector Velocity;

	// Estados do MovementComponent
	UPROPERTY(BlueprintReadWrite)
	bool bIsFalling;

	UPROPERTY(BlueprintReadWrite)
	bool bIsCrouched;

	// Se você quiser, pode armazenar MovementMode (MOVE_Walking, MOVE_Falling, etc.)
	UPROPERTY(BlueprintReadWrite)
	uint8 MovementMode;
    
	// Timestamp, se precisar de interpolação ou algo do tipo
	UPROPERTY(BlueprintReadWrite)
	float TimeStamp;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PUZZLE_API UGhostComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGhostComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void iterateTransform();
	void Capture();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int captureInterval = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float captureTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ReplayCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FTransform> TransformArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AActor* FollowTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int CurrentTransformIndex = 0;

	float ElapsedTime = 0.0f;
	float TickAccumulator = 0.0f ;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsCapturing = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsPlaying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int ReplayCounter;

	// GhostComponent.h
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FMovementSnapshot> MovementSnapshots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<ECollisionResponse>> CollisionResponses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* TargetIconActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> TargetIconActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class AGhostReplayer* ParentGhostReplayer;

	void SpawnTimer();
	
	


	void StartReplay();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

		
};
