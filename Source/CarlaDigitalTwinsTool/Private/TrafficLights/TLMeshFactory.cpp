#include "TrafficLights/TLMeshFactory.h"
#include "TrafficLights/TLModuleDataTable.h"
#include "TrafficLights/TLPoleDataTable.h"

UDataTable *FTLMeshFactory::ModuleMeshTable{nullptr};
UDataTable *FTLMeshFactory::PoleMeshTable{nullptr};
UDataTable *FTLMeshFactory::LightTypeMeshTable{nullptr};

UDataTable *FTLMeshFactory::GetLightTypeMeshTable() {
  if (!LightTypeMeshTable) {
    constexpr TCHAR const *Path{
        TEXT("/Game/Carla/Static/TrafficLight/TrafficLights2025/"
             "DataTables/LightTypes.LightTypes")};
    UObject *Loaded =
        StaticLoadObject(UDataTable::StaticClass(), nullptr, Path);
    LightTypeMeshTable = Cast<UDataTable>(Loaded);
  }
  return LightTypeMeshTable;
}

UDataTable *FTLMeshFactory::GetModuleMeshTable() {
  if (!ModuleMeshTable) {
    constexpr TCHAR const *Path{
        TEXT("/Game/Carla/Static/TrafficLight/TrafficLights2025/DataTables/"
             "Modules.Modules")};
    UObject *Loaded =
        StaticLoadObject(UDataTable::StaticClass(), nullptr, Path);
    ModuleMeshTable = Cast<UDataTable>(Loaded);
  }
  return ModuleMeshTable;
}

UDataTable *FTLMeshFactory::GetPoleMeshTable() {
  if (!PoleMeshTable) {
    constexpr TCHAR const *Path{
        TEXT("/Game/Carla/Static/TrafficLight/TrafficLights2025/DataTables/"
             "Poles.Poles")};
    UObject *Loaded =
        StaticLoadObject(UDataTable::StaticClass(), nullptr, Path);
    PoleMeshTable = Cast<UDataTable>(Loaded);
  }
  return PoleMeshTable;
}

UStaticMesh *FTLMeshFactory::GetMeshForModule(const FTLHead &Head,
                                              const FTLModule &Module) {
  UDataTable *ModuleMeshTable{GetModuleMeshTable()};
  if (!ModuleMeshTable) {
    UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: ModuleMeshTable is null"));
    return nullptr;
  }

  // Get rows
  const auto Rows{ModuleMeshTable->GetRowNames()};
  if (Rows.Num() == 0) {
    UE_LOG(LogTemp, Error,
           TEXT("ModuleMeshFactory: no rows found in ModuleMeshTable"));
    return nullptr;
  }

  for (const FName &RowName : Rows) {
    if (const FTLModuleRow *Row = ModuleMeshTable->FindRow<FTLModuleRow>(
            RowName, TEXT("GetMeshForModule"))) {
      if (!Row) {
        UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: row '%s' not found"),
               *RowName.ToString());
        continue;
      }
      if (Row->Style == Head.Style && Row->Orientation == Head.Orientation &&
          Row->bHasVisor == Module.bHasVisor) {
        if (Row->Mesh) {
          return Row->Mesh;
        }
      }
    }
  }

  return nullptr;
}

TArray<UStaticMesh *>
FTLMeshFactory::GetAllMeshesForModule(const FTLHead &Head,
                                      const FTLModule &Module) {
  TArray<UStaticMesh *> Meshes;
  UDataTable *ModuleMeshTable{GetModuleMeshTable()};
  if (!ModuleMeshTable) {
    UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: ModuleMeshTable is null"));
    return Meshes;
  }

  // Get rows
  const auto Rows{ModuleMeshTable->GetRowNames()};
  if (Rows.Num() == 0) {
    UE_LOG(LogTemp, Error,
           TEXT("ModuleMeshFactory: no rows found in ModuleMeshTable"));
    return Meshes;
  }

  for (const FName &RowName : Rows) {
    if (const FTLModuleRow *Row = ModuleMeshTable->FindRow<FTLModuleRow>(
            RowName, TEXT("GetMeshForModule"))) {
      if (!Row) {
        UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: row '%s' not found"),
               *RowName.ToString());
        continue;
      }
      if (Row->Style == Head.Style && Row->Orientation == Head.Orientation &&
          Row->bHasVisor == Module.bHasVisor) {
        if (Row->Mesh) {
          Meshes.Add(Row->Mesh);
        }
      }
    }
  }

  return Meshes;
}

UStaticMesh *FTLMeshFactory::GetBaseMeshForPole(const FTLPole &Pole) {
  TArray<UStaticMesh *> All = GetAllBaseMeshesForPole(Pole);
  return All.Num() ? All[0] : nullptr;
}

UStaticMesh *FTLMeshFactory::GetExtendibleMeshForPole(const FTLPole &Pole) {
  TArray<UStaticMesh *> All = GetAllExtendibleMeshesForPole(Pole);
  return All.Num() ? All[0] : nullptr;
}

UStaticMesh *FTLMeshFactory::GetFinalMeshForPole(const FTLPole &Pole) {
  TArray<UStaticMesh *> All = GetAllFinalMeshesForPole(Pole);
  return All.Num() ? All[0] : nullptr;
}

TArray<UStaticMesh *>
FTLMeshFactory::GetAllBaseMeshesForPole(const FTLPole &Pole) {
  TArray<UStaticMesh *> Meshes;
  UDataTable *Table = GetPoleMeshTable();
  if (!Table) {
    UE_LOG(LogTemp, Error, TEXT("PoleMeshFactory: PoleMeshTable is null"));
    return Meshes;
  }

  for (const FName &RowName : Table->GetRowNames()) {
    const FTLPoleRow *Row =
        Table->FindRow<FTLPoleRow>(RowName, TEXT("GetAllBaseMeshesForPole"));
    if (!Row) {
      UE_LOG(LogTemp, Warning, TEXT("PoleMeshFactory: row '%s' not found"),
             *RowName.ToString());
      continue;
    }

    if (Row->Style == Pole.Style && Row->Orientation == Pole.Orientation) {
      if (Row->BaseMesh) {
        Meshes.Add(Row->BaseMesh);
      }
    }
  }
  return Meshes;
}

TArray<UStaticMesh *>
FTLMeshFactory::GetAllExtendibleMeshesForPole(const FTLPole &Pole) {
  TArray<UStaticMesh *> Meshes;
  UDataTable *Table = GetPoleMeshTable();
  if (!Table) {
    UE_LOG(LogTemp, Error, TEXT("PoleMeshFactory: PoleMeshTable is null"));
    return Meshes;
  }

  for (const FName &RowName : Table->GetRowNames()) {
    const FTLPoleRow *Row = Table->FindRow<FTLPoleRow>(
        RowName, TEXT("GetAllExtendibleMeshesForPole"));
    if (!Row) {
      UE_LOG(LogTemp, Warning, TEXT("PoleMeshFactory: row '%s' not found"),
             *RowName.ToString());
      continue;
    }

    if (Row->Style == Pole.Style && Row->Orientation == Pole.Orientation) {
      if (Row->ExtendibleMesh) {
        Meshes.Add(Row->ExtendibleMesh);
      }
    }
  }
  return Meshes;
}

TArray<UStaticMesh *>
FTLMeshFactory::GetAllFinalMeshesForPole(const FTLPole &Pole) {
  TArray<UStaticMesh *> Meshes;
  UDataTable *Table = GetPoleMeshTable();
  if (!Table) {
    UE_LOG(LogTemp, Error, TEXT("PoleMeshFactory: PoleMeshTable is null"));
    return Meshes;
  }

  for (const FName &RowName : Table->GetRowNames()) {
    const FTLPoleRow *Row =
        Table->FindRow<FTLPoleRow>(RowName, TEXT("GetAllFinalMeshesForPole"));
    if (!Row) {
      UE_LOG(LogTemp, Warning, TEXT("PoleMeshFactory: row '%s' not found"),
             *RowName.ToString());
      continue;
    }

    if (Row->Style == Pole.Style && Row->Orientation == Pole.Orientation) {
      if (Row->FinalMesh) {
        Meshes.Add(Row->FinalMesh);
      }
    }
  }
  return Meshes;
}
