// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinimapWidget.h"
#include "UMG.h"
#include "Minimap.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMinimap, Log, All);

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
	void OnActorSpawned(AActor *actor);

	UCanvasPanel *minimap_panel;
	TArray<TPair<AActor*, UCanvasPanelSlot*>> tracked_actors;
	UWorld *world;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Set this to the minimap widget instance, and make sure that it
	// implements GetMinimapIconArea() to return the panel which will
	// contain the minimap icons
	UPROPERTY(BlueprintReadWrite, Category = "Minimap")
	class UMinimapWidget *minimap_widget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TMap<UClass*, UTexture2D*> legend;
};
