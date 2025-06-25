// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"

#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"
#include "TrafficLights/TLPole.h"

class FTLMeshFactory {
public:
  static UStaticMesh *GetMeshForModule(const FTLHead &Head,
                                       const FTLModule &Module);
  static TArray<UStaticMesh *> GetAllMeshesForModule(const FTLHead &Head,
                                                     const FTLModule &Module);
  static UStaticMesh *GetBaseMeshForPole(const FTLPole &Pole);
  static UStaticMesh *GetExtendibleMeshForPole(const FTLPole &Pole);
  static UStaticMesh *GetFinalMeshForPole(const FTLPole &Pole);
  static TArray<UStaticMesh *> GetAllBaseMeshesForPole(const FTLPole &Pole);
  static TArray<UStaticMesh *>
  GetAllExtendibleMeshesForPole(const FTLPole &Pole);
  static TArray<UStaticMesh *> GetAllFinalMeshesForPole(const FTLPole &Pole);

private:
  static UDataTable *GetModuleMeshTable();
  static UDataTable *GetPoleMeshTable();

private:
  static UDataTable *ModuleMeshTable;
  static UDataTable *PoleMeshTable;
};
