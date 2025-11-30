#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostReplayer.generated.h"

USTRUCT(BlueprintType)
struct FGhostsArray
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ghosts")
	TArray<AActor*> Ghosts;
};

UCLASS()
class PUZZLE_API AGhostReplayer : public AActor
{
	GENERATED_BODY()

public:
	AGhostReplayer();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	void NotifyActorBeginOverlap(AActor* OtherActor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBoxComponent* Detection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStaticMeshComponent* BoxMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int captureInterval = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float captureTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int DefaultReplayCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<AActor*, FTransform> TrackedActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<AActor*, FGhostsArray> GhostActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<AActor*, FTimerHandle> ActorTimers;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AActor*> OverlappingActors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* GhostMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* PostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> TargetIconActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<ECollisionResponse>> CollisionResponses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Replayer")
	bool bIsEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "Ghost Replayer", CallInEditor)
	void SetEnabled(bool bEnabled);
	

private:	
	
	void NotifyActorEndOverlap(AActor* OtherActor);
	void StopTrackingActor(AActor* Actor);
	void StartReplay(AActor* Actor);
	void RestartActorPosition(AActor* Actor);

	void SpawnGhost(AActor* Actor);
	void DestroyGhostsForActor(AActor* Actor);

	bool IsGhostActor(AActor* Actor) const;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
};
