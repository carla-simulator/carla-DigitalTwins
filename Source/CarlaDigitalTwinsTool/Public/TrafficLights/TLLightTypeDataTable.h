// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Engine/DataTable.h"
#include "Math/MathFwd.h"
#include "UObject/ObjectMacros.h"

#include "TLLightTypeDataTable.generated.h"

USTRUCT(BlueprintType)
struct FTLLightTypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|LightType")
	FIntPoint AtlasCoords;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic Light|LightType")
	FLinearColor Color;
};
