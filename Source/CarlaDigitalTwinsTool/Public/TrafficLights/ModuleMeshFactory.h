// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"


class FModuleMeshFactory
{
public:
    static UStaticMesh* GetMeshForModule(const struct FTLHead& Head, const struct FTLModule& Module);
};
