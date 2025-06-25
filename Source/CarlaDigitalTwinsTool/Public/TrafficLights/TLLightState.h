// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "HAL/Platform.h"
#include "UObject/ObjectMacros.h"

#include "TLLightState.generated.h"

UENUM(BlueprintType)
enum class ETLLightState : uint8
{
	Inactive UMETA(DisplayName = "Off"),
	Solid UMETA(DisplayName = "On"),
	Flashing UMETA(DisplayName = "Flashing")
};
