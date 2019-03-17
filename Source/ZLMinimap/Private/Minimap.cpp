// Fill out your copyright notice in the Description page of Project Settings.

#include "Minimap.h"
#include "MinimapIconComponent.h"

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

	world = GetWorld();
	if (!world) {
		// I don't know if this can happen, but it's a pointer, let's be safe
		UE_LOG(LogMinimap, Warning, TEXT("No world"));
		return;
	}
	world->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &AMinimap::OnActorSpawned));

	for (TActorIterator<AActor> it(world); it; ++it) {
		OnActorSpawned(*it);
	}
}

void AMinimap::OnActorSpawned(AActor *actor)
{
	UTexture2D **texture = nullptr;

	// Find an icon for this actor, either attached to the actor's
	// MinimapIconComponent, or from the minimap class legend:
	UMinimapIconComponent *actor_icon = actor->FindComponentByClass<UMinimapIconComponent>();
	if (actor_icon)
		texture = &actor_icon->minimap_icon;
	else
		texture = legend.Find(actor->GetClass());

	if (!texture || !*texture)
		return;

	UImage *icon = NewObject<UImage>();
	icon->SetBrushFromTexture(*texture);
	UCanvasPanelSlot *slot = Cast<UCanvasPanelSlot>(minimap_panel->AddChild(icon));
	slot->SetAlignment(FVector2D(0.5, 0.5));
	slot->SetSize(FVector2D((*texture)->GetSizeX(), (*texture)->GetSizeY()));
	tracked_actors.Emplace(actor, slot);
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
	APawn *pawn;

	Super::Tick(DeltaTime);

	if (!minimap_panel)
		return;

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

	for (int i = tracked_actors.Num() - 1; i >= 0; --i) {
		AActor *actor = tracked_actors[i].Key;
		UCanvasPanelSlot *slot = tracked_actors[i].Value;

		if (!actor || !actor->CheckStillInWorld() || !slot) {
			// Actor destroyed or someone messed with
			// our panel while we weren't looking
			if (slot)
				slot->Content->RemoveFromParent();
			tracked_actors.RemoveAt(i);
			continue;
		}

		FVector2D icon_loc = FVector2D(actor->GetActorLocation()) - player_loc;
		// TODO: Check if possibly in range before applying rotation
		slot->SetPosition(icon_loc.GetRotated(270 - camera_rot.Yaw) * panel_scale + panel_pivot);
	}
}

