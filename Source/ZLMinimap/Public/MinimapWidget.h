// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class ZLMINIMAP_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	// Assign the panel area that will be used to display minimap icons via this:
	UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
	UCanvasPanel *GetMinimapIconArea() const;
};
