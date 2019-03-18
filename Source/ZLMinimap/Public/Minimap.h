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
	UImage *minimap_background;
	TArray<TPair<AActor*, UCanvasPanelSlot*>> tracked_actors;
	UWorld *world;
	FVector camera_offset;
	FVector2D cached_panel_size;
	FVector2D cached_panel_pivot;
	float cached_panel_scale;
	int32 max_icon_size;
	float max_distance_from_player;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Call this if any properties are updated at runtime
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void UpdateCachedProperties();

	void TrackActor(AActor *actor, UTexture2D *icon);

	// Set this to the minimap widget instance, and make sure that it
	// implements GetMinimapIconArea() to return the panel which will
	// contain the minimap icons
	UPROPERTY(BlueprintReadWrite, Category = "Minimap")
	class UMinimapWidget *minimap_widget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TMap<UClass*, UTexture2D*> legend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	USceneCaptureComponent2D *background_capture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTextureRenderTarget2D *background_rt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float minimap_scale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float camera_height;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D minimap_center;
};
