#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "FollowerTimer.generated.h"

UCLASS()
class PUZZLE_API AFollowerTimer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFollowerTimer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextRenderComponent* Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Percentage = 0.0f;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UpdatePercentage(float InPercentage);

};
