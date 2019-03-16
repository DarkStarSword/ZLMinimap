// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinimapWidget.h"
#include "Minimap.generated.h"
UCLASS()
class ZLMINIMAP_API AMinimap : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMinimap();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Set this to the minimap widget:
	UPROPERTY(EditAnywhere)
	class UMinimapWidget *minimap_widget;
	
	UPROPERTY(EditAnywhere)
	UPanelWidget *minimap_panel;
};
