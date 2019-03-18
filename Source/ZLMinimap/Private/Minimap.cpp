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
	// Track player rotation by default:
	lock_yaw_to_camera = true;
	add_all_static_actors_to_background = true;

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

void AMinimap::TrackActor(AActor *actor, UTexture2D *texture)
{
	int32 width = texture->GetSizeX();
	int32 height = texture->GetSizeY();

	UImage *icon = NewObject<UImage>();
	icon->SetBrushFromTexture(texture);
	UCanvasPanelSlot *slot = Cast<UCanvasPanelSlot>(minimap_panel->AddChild(icon));
	slot->SetAlignment(FVector2D(0.5, 0.5));
	slot->SetSize(FVector2D(width, height));
	tracked_actors.Emplace(actor, slot);

	max_icon_size = FMath::Max<uint32>(max_icon_size, FMath::Max<uint32>(width, height));
}

void AMinimap::OnActorSpawned(AActor *actor)
{
	UTexture2D **texture = nullptr;

	// Find an icon for this actor, either attached to the actor's
	// MinimapIconComponent, or from the minimap class legend:
	UMinimapIconComponent *actor_icon = actor->FindComponentByClass<UMinimapIconComponent>();
	if (actor_icon) {
		texture = &actor_icon->minimap_icon;
		if (background_capture && actor_icon->show_on_background)
			background_capture->ShowOnlyActors.Add(actor);
	} else {
		texture = legend.Find(actor->GetClass());
		if (add_all_static_actors_to_background && background_capture && actor->IsRootComponentStatic())
			background_capture->ShowOnlyActors.Add(actor);
	}

	if (texture && *texture)
		TrackActor(actor, *texture);
}

// Called to update cached properties so we don't have to do this every frame:
void AMinimap::UpdateCachedProperties()
{
	APlayerController *player;

	// If we aren't following a specific actor, follow the player:
	if (!follow_actor) {
		player = world->GetFirstPlayerController();
		if (player)
			follow_actor = player->GetPawn();
	}

	// Update intermediate values that depend on the UI size:
	cached_panel_size = minimap_panel->GetCachedGeometry().GetLocalSize();
	cached_panel_pivot = cached_panel_size * minimap_center;
	cached_panel_scale = cached_panel_size.X / minimap_scale;

	// Determine the maximum distance an actor can be from
	// the player to consider drawing their minimap icons,
	// allowing us to skip rotation calculations for any
	// actors clearly too far out of range. We're only
	// going to compare whichever corner of the minimap is
	// furthest from the center to reduce the number of
	// checks that we perform:
	float icon_padding = max_icon_size / cached_panel_size.X / 2.0f;
	float from_left_or_right = FVector2D(minimap_center.X, 1.0 - minimap_center.X).GetAbsMax() + icon_padding;
	float from_top_or_bottom = FVector2D(minimap_center.Y, 1.0 - minimap_center.Y).GetAbsMax() + icon_padding;
	float from_furthest_corner = FVector2D::Distance(FVector2D(from_left_or_right, from_top_or_bottom), FVector2D(0,0));
	max_distance_from_player = from_furthest_corner * minimap_scale;

	// Synchronise the capture component's zoom with ours and make
	// sure that it captures the background at the new scale:
	if (background_capture) {
		background_capture->OrthoWidth = minimap_scale;
		background_capture->CaptureScene();
	}
}

void AMinimap::UpdatePosition()
{
	FRotator camera_rot;
	FVector camera_pos;

	if (!follow_actor)
		return;

	cached_target_location = follow_actor->GetActorLocation();

	if (lock_yaw_to_camera) {
		follow_actor->GetActorEyesViewPoint(camera_pos, camera_rot);
		minimap_yaw = camera_rot.Yaw;
	}

	// We track the actor, so the capture component's position will too.
	// Add an offset for the minimap pivot back into world space, so that
	// the capture component will be lined up with the center of the
	// minimap, even when the player is offset from that:
	FVector capture_loc = cached_target_location;
	capture_loc += FVector((minimap_center - FVector2D(0.5, 0.5)).GetRotated(minimap_yaw - 90) * minimap_scale, camera_height);
	// Look down for a top-down view, and match the player camera rotation
	// yaw to keep up/north pointing in the same direction as the player:
	SetActorLocationAndRotation(capture_loc, FRotator(-90.0f, minimap_yaw, 0));
}

// Called every frame
void AMinimap::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!minimap_panel || !world)
		return;

	if (cached_panel_size != minimap_panel->GetCachedGeometry().GetLocalSize())
		UpdateCachedProperties();

	UpdatePosition();

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

		FVector2D icon_loc = FVector2D(actor->GetActorLocation()) - FVector2D(cached_target_location);
		// Cull objects clearly out of range to skip the rotation calculation:
		// if (FVector2D::Distance(icon_loc, FVector2D(0, 0)) < max_distance_from_player) {
		// except we don't want to do a distance calculation either, so we just compare
		// the max X or Y from the player, which is more than enough to make sure everything
		// that should be on the minimap shows regardless of rotation, while culling things
		// that are way too far off, and relying on the panel to clip anything in between:
		if (icon_loc.GetAbsMax() < max_distance_from_player) {
			slot->SetPosition(icon_loc.GetRotated(270 - minimap_yaw) * cached_panel_scale + cached_panel_pivot);
			slot->Content->SetVisibility(ESlateVisibility::Visible);
		} else {
			slot->Content->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

