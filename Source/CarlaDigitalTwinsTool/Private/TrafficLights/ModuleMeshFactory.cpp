#include "TrafficLights/ModuleMeshFactory.h"
#include "UObject/SoftObjectPath.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"
#include "TrafficLights/TLHeadOrientation.h"
#include "TrafficLights/TLHeadStyle.h"
#include "UObject/ConstructorHelpers.h"

UDataTable* FModuleMeshFactory::ModuleMeshTable = nullptr;

UDataTable* FModuleMeshFactory::GetModuleMeshTable()
{
    if (!ModuleMeshTable)
    {
        constexpr TCHAR const* Path {TEXT("/Game/Carla/DataTables/Modules.Modules")};
        UObject* Loaded = StaticLoadObject(UDataTable::StaticClass(), nullptr, Path);
        ModuleMeshTable = Cast<UDataTable>(Loaded);
        if (!ModuleMeshTable)
        {
            UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: no pude cargar el DataTable en '%s'"), Path);
        }
    }
    return ModuleMeshTable;
}

UStaticMesh* FModuleMeshFactory::GetMeshForModule(const FTLHead& Head, const FTLModule& Module)
{
    UDataTable* ModuleMeshTable {GetModuleMeshTable()};
    if (!ModuleMeshTable)
    {
        UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: ModuleMeshTable is null"));
        return nullptr;
    }

    // Get rows
    const auto Rows {ModuleMeshTable->GetRowNames()};
    if (Rows.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: no rows found in ModuleMeshTable"));
        return nullptr;
    }

    for (const FName& RowName : Rows)
    {
        if (const FTLModuleRow* Row = ModuleMeshTable->FindRow<FTLModuleRow>(RowName, TEXT("GetMeshForModule")))
        {
            if (!Row)
            {
                UE_LOG(LogTemp, Error, TEXT("ModuleMeshFactory: row '%s' not found"), *RowName.ToString());
                continue;
            }
            if (Row->Style == Head.Style && Row->Orientation == Head.Orientation && Row->bHasVisor == Module.bHasVisor)
            {
                if (Row->Mesh)
                {
                    return Row->Mesh;
                }
            }
        }
    }

    return nullptr;
}
