// Fill out your copyright notice in the Description page of Project Settings.


#include "Cable.h"

#include "FrameTypes.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ACable::ACable()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(RootComponent);
	Spline->SetMobility(EComponentMobility::Movable);

}


void ACable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CreateSplineMesh();
}

void ACable::CreateSplineMesh()
{
	for (USplineMeshComponent* SplineMesh : SplineMeshes)
	{
		if (SplineMesh && SplineMesh->IsValidLowLevel())
		{
			SplineMesh->DestroyComponent();
		}
	}

	// Limpa o array após destruir os componentes
	SplineMeshes.Empty();
	for (USplineMeshComponent* SplineMesh : SplineEdgeMeshes)
	{
		if (SplineMesh && SplineMesh->IsValidLowLevel())
		{
			SplineMesh->DestroyComponent();
		}
	}

	// Limpa o array após destruir os componentes
	SplineEdgeMeshes.Empty();

	for (int i = 0; i <= Spline->GetNumberOfSplinePoints() - 2; i++)
	{

		UMaterialInstanceDynamic *MaterialInstance = UMaterialInstanceDynamic::Create(CableMaterial, this);
		MaterialInstance->SetVectorParameterValue("PowerOn", PowerOnColor);
		MaterialInstance->SetVectorParameterValue("PowerOff", PowerOffColor);
		UMaterialInstanceDynamic *MaterialInstanceEdge = UMaterialInstanceDynamic::Create(CableMaterialEdge, this);



		FVector StartLocation, StartTangent, EndLocation, EndTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, StartLocation, StartTangent, ESplineCoordinateSpace::World);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, EndLocation, EndTangent, ESplineCoordinateSpace::World);

		USplineMeshComponent* SplineMeshComponent = AddSplineMeshComponent();
		if(!SplineMeshComponent)
		{
			continue;
		}
		// **Set the mobility to Movable** 
		SplineMeshComponent->SetMobility(EComponentMobility::Movable);
		SplineMeshComponent->ForwardAxis = ESplineMeshAxis::Z;
		SplineMeshComponent->SetStartScale(CableStartScale);
		SplineMeshComponent->SetEndScale(CableEndScale);
		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMeshComponent->SetStaticMesh(CableMesh);
		SplineMeshComponent->SetMaterial(0, MaterialInstance);
		//SplineMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		SplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);

		USplineMeshComponent* SplineMeshComponentEdge = AddSplineMeshComponent();
		SplineMeshComponentEdge->ForwardAxis = ESplineMeshAxis::Z;
		SplineMeshComponentEdge->SetStartScale(FVector2D(CableStartScale.X * 2, CableStartScale.Y / 2));
		SplineMeshComponentEdge->SetEndScale(FVector2D(CableStartScale.X * 2, CableStartScale.Y / 2));
		SplineMeshComponentEdge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//SplineMeshComponentEdge->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		SplineMeshComponentEdge->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);
		SplineMeshComponentEdge->SetStaticMesh(CableMesh);
		SplineMeshComponentEdge->SetMaterial(0, MaterialInstanceEdge);

		SplineMeshes.Add(SplineMeshComponent);
		SplineEdgeMeshes.Add(SplineMeshComponentEdge);
		
		
	}
}

void ACable::PowerCableVisuals()
{
	for (int i = 1; i < Spline->GetNumberOfSplinePoints(); i++)
	{
		float PointDistance = Spline->GetDistanceAlongSplineAtSplinePoint(i-1);

		// Se a distância ainda não alcançou o ponto atual, pule para a próxima iteração
		if (PointDistance > distance)
		{
			continue;
		}

		// Pegue a SplineMesh atual (i - 1)
		USplineMeshComponent* SplineMesh = SplineMeshes[i - 1];
		UMaterialInterface* m = SplineMesh->GetMaterial(0);
		UMaterialInstanceDynamic* m_dyn = Cast<UMaterialInstanceDynamic>(m);

		if (m_dyn)
		{
			// Calcula o preenchimento da SplineMesh (de 1 a 0)
			float Progress = 1.0f - FMath::Clamp((distance - PointDistance) / (Spline->GetDistanceAlongSplineAtSplinePoint(i) - PointDistance), 0.0f, 1.0f);
            
			m_dyn->SetScalarParameterValue("Percentage", Progress);
		}
	}
}

// Called when the game starts or when spawned
void ACable::BeginPlay()
{
	Super::BeginPlay();
	CreateSplineMesh();
	
}

// Called every frame
void ACable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bIsFilling)
	{
		distance = distance + (fillSpeed * DeltaTime);
		if(distance >= Spline->GetSplineLength())
		{
			distance = Spline->GetSplineLength();
		}
	} else
	{
		distance = distance - (fillSpeed * DeltaTime);
		if(bIsPowered)
		{
			bIsPowered = false;
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Power Off");
			OnPowerOff();
		}
		if(distance <= 0)
		{
			distance = 0;
		}
	}
	
	if(distance >= Spline->GetSplineLength() && !bIsPowered)
	{
		bIsPowered = true;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Power On");
		OnPowerOn();
	} 
	PowerCableVisuals();
}

