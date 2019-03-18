// Fill out your copyright notice in the Description page of Project Settings.

#include "Minimap.h"
#include "MinimapIconComponent.h"

DEFINE_LOG_CATEGORY(LogMinimap);

// Sets default values
AMinimap::AMinimap()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Default to offsetting the center to be 1/4 from the bottom:
	minimap_center = FVector2D(0.5f, 0.75f);
	// Height of the background camera above the player to get a good top-down view:
	camera_height = 1000.0f;
	// Default horizontal diameter of the world the minimap covers:
	minimap_scale = 2048.0f;

	background_capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapBackgroundCapture"));
	background_capture->SetupAttachment(RootComponent);
	background_capture->TextureTarget = background_rt;
	background_capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	background_capture->ProjectionType = ECameraProjectionMode::Orthographic;
	background_capture->OrthoWidth = minimap_scale;
	background_capture->bCaptureEveryFrame = false;
	background_capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
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

	// minimap_background = minimap_widget->GetMinimapBackgroundImage();
	if (background_capture && background_rt /* && minimap_background */) {
		background_capture->TextureTarget = background_rt;
		//TODO: Set the minimap_background brush to the render target
		//      then we could just create the render target dynamically
		//      at this point with optimal size to match the UI.
	} else if (!background_capture) {
		UE_LOG(LogMinimap, Warning, TEXT("Minimap background is not set up (capture)"));
	} else if (!background_rt) {
		UE_LOG(LogMinimap, Warning, TEXT("Minimap background is not set up (rt)"));
	} else if (!minimap_background) {
		UE_LOG(LogMinimap, Warning, TEXT("Minimap background is not set up (widget)"));
	}

	// Process all current and future actors, adding minimap icons for any
	// in our class legend or that have attached minimap icon components:
	world->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &AMinimap::OnActorSpawned));
	for (TActorIterator<AActor> it(world); it; ++it) {
		OnActorSpawned(*it);
	}
}

void AMinimap::track_actor(AActor *actor, UTexture2D *texture)
{
	UImage *icon = NewObject<UImage>();
	icon->SetBrushFromTexture(texture);
	UCanvasPanelSlot *slot = Cast<UCanvasPanelSlot>(minimap_panel->AddChild(icon));
	slot->SetAlignment(FVector2D(0.5, 0.5));
	slot->SetSize(FVector2D(texture->GetSizeX(), texture->GetSizeY()));
	tracked_actors.Emplace(actor, slot);
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

	if (texture && *texture)
		track_actor(actor, *texture);

	if (background_capture && actor->IsRootComponentStatic())
		background_capture->ShowOnlyActors.Add(actor);
}

// Called every frame
void AMinimap::Tick(float DeltaTime)
{
	APlayerController *player;
	FVector2D panel_pivot;
	FVector2D panel_size;
	FRotator camera_rot;
	FVector camera_pos;
	FVector player_loc;
	float panel_scale = 1.0f;
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

	panel_size = minimap_panel->GetCachedGeometry().GetLocalSize();
	panel_pivot = panel_size * minimap_center;
	panel_scale = panel_size.X / minimap_scale;

	player->GetPlayerViewPoint(camera_pos, camera_rot);
	player_loc = pawn->GetActorLocation();

	// We track the actor, so the capture component's position will too.
	// Add an offset for the minimap pivot back into world space, so that
	// the capture component will be lined up with the center of the
	// minimap, even when the player is offset from that:
	FVector capture_loc = player_loc + FVector((minimap_center - FVector2D(0.5, 0.5)).GetRotated(camera_rot.Yaw - 90) * minimap_scale, camera_height);
	// Look down for a top-down view, and match the player camera rotation
	// yaw to keep up/north pointing in the same direction as the player:
	SetActorLocationAndRotation(capture_loc, FRotator(-90.0f, camera_rot.Yaw, 0));

	// Synchronise the capture component's zoom with ours:
	if (background_capture)
		background_capture->OrthoWidth = minimap_scale;

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

		FVector2D icon_loc = FVector2D(actor->GetActorLocation()) - FVector2D(player_loc);
		// TODO: Check if possibly in range before applying rotation
		slot->SetPosition(icon_loc.GetRotated(270 - camera_rot.Yaw) * panel_scale + panel_pivot);
	}
}

