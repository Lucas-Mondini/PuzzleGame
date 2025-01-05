// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cable.generated.h"

class USplineMeshComponent;
class USplineComponent;

UCLASS()
class PUZZLE_API ACable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACable();

	//linked actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> LinkedActors;

	
	UPROPERTY(EditAnywhere, Transient, BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	bool bIsFilling = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPowered = false;

	UPROPERTY(EditAnywhere)
	float distance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float fillSpeed = 100.f;


	UPROPERTY(EditAnywhere)
	FVector2D CableStartScale = FVector2D(0.06, 0.05);
	UPROPERTY(EditAnywhere)
	FVector2D CableEndScale = FVector2D(0.05, 0.06);
	

	UPROPERTY(EditAnywhere, Category = "Color")
	FColor PowerOnColor;
	UPROPERTY(EditAnywhere, Category = "Color")
	FColor PowerOffColor;

	UPROPERTY(EditAnywhere)
	TArray<USplineMeshComponent*> SplineMeshes;

	UPROPERTY(EditAnywhere)
	TArray<USplineMeshComponent*> SplineEdgeMeshes;
	
	UPROPERTY(EditAnywhere)
	USplineComponent* Spline;
	UPROPERTY(EditAnywhere)
	UMaterialInstance *CableMaterial;
	UPROPERTY(EditAnywhere)
	UMaterialInstance *CableMaterialEdge;
	
	UPROPERTY(EditAnywhere)
	UStaticMesh* CableMesh;
	

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable)
	void CreateSplineMesh();

	UFUNCTION()
	void PowerCableVisuals();

	UFUNCTION(BlueprintImplementableEvent)
	USplineMeshComponent* AddSplineMeshComponent();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPowerOn();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPowerOff();
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
