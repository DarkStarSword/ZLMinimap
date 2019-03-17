// Fill out your copyright notice in the Description page of Project Settings.

#include "Minimap.h"

DEFINE_LOG_CATEGORY(LogMinimap);

// Sets default values
AMinimap::AMinimap()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMinimap::BeginPlay()
{
	Super::BeginPlay();

	if (!minimap_widget) {
		UE_LOG(LogMinimap, Warning, TEXT("No minimap widget assigned"));
		return;
	}

	// Minimap widget currently added to the viewport via blueprint to
	// allow flexibility if it was wanted somewhere other than the viewport

	minimap_panel = minimap_widget->GetMinimapIconArea();
	if (!minimap_panel) {
		UE_LOG(LogMinimap, Warning, TEXT("Minimap widget's icon area panel is not set"));
		return;
	}
}

// Called every frame
void AMinimap::Tick(float DeltaTime)
{
	APlayerController *player;
	FVector2D panel_pivot;
	FVector2D panel_size;
	FVector2D player_loc;
	FRotator camera_rot;
	FVector camera_pos;
	float panel_scale;
	UWorld *world;
	APawn *pawn;

	Super::Tick(DeltaTime);

	if (!minimap_panel)
		return;

	// For now I'm adding new icons and iterating all actors each frame.
	// Once the transformation works I'll optimise this
	minimap_panel->ClearChildren();
	// TODO: world->AddOnActorSpawnedHandler

	world = GetWorld();
	if (!world)
		return;

	// TODO: Allow the minimap to be centered on an arbitrary actor
	player = world->GetFirstPlayerController();
	if (!player)
		return;

	pawn = player->GetPawn();
	if (!pawn)
		return;

	// FIXME: Tie vertical offset and render size with screen capture component:
	panel_size = minimap_panel->GetCachedGeometry().GetLocalSize();
	panel_pivot = panel_size * FVector2D(0.5, 0.75);
	panel_scale = panel_size.X / 2048.0f;

	player->GetPlayerViewPoint(camera_pos, camera_rot);
	player_loc = FVector2D(pawn->GetActorLocation());

	for (TActorIterator<AActor> it(world); it; ++it) {
		UImage *icon = NewObject<UImage>();
		UCanvasPanelSlot *slot = Cast<UCanvasPanelSlot>(minimap_panel->AddChild(icon));
		FVector2D icon_loc = FVector2D(it->GetActorLocation()) - player_loc;
		// TODO: Check if possibly in range before applying rotation
		slot->SetPosition(icon_loc.GetRotated(270 - camera_rot.Yaw) * panel_scale + panel_pivot);
		slot->SetAlignment(FVector2D(0.5, 0.5));
		slot->SetSize(FVector2D(20, 20));
	}
}

