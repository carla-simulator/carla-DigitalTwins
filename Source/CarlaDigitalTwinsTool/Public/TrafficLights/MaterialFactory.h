// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
#include "TrafficLights/TLLightType.h"

struct FTLHead;
struct FTLModule;

class FMaterialFactory
{
public:
    static UMaterialInterface* GetModuleBodyMaterial(const FTLHead& Head, const FTLModule& Module);

    static UMaterialInterface* GetLightMaterial(ETLLightType LightType);
};
