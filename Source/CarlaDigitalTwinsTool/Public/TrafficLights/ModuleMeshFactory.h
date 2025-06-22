// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "TrafficLights/TLModuleDataTable.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"

class FModuleMeshFactory
{
public:
    static UStaticMesh* GetMeshForModule(const FTLHead& Head, const FTLModule& Module);
    static TArray<UStaticMesh*> GetAllMeshesForModule(const FTLHead& Head, const FTLModule& Module);

private:
    static UDataTable* GetModuleMeshTable();

private:
    static UDataTable* ModuleMeshTable;
};
