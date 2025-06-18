// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TrafficLights/TLHeadStyle.h"
#include "TrafficLights/TLHeadOrientation.h"
#include "TLModuleDataTable.generated.h"

USTRUCT(BlueprintType)
struct FTLModuleRow : public FTableRowBase
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Traffic Light|Module")
  ETLHeadStyle Style;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Traffic Light|Module")
  ETLHeadOrientation Orientation;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Traffic Light|Module")
  bool bHasVisor;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Traffic Light|Module")
  FString Path;
};
