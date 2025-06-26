// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "TrafficLights/TLOrientation.h"
#include "TrafficLights/TLStyle.h"
#include "UObject/ObjectMacros.h"

#include "TLPoleDataTable.generated.h"

USTRUCT(BlueprintType)
struct FTLPoleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light|Pole")
	ETLStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|Pole")
	ETLOrientation Orientation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|Pole")
	UStaticMesh* BaseMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|Pole")
	UStaticMesh* ExtendibleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|Pole")
	UStaticMesh* CapMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|Pole")
	UStaticMesh* ConnectorMesh;
};
