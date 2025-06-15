#include "TrafficLights/ModuleMeshFactory.h"
#include "UObject/SoftObjectPath.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"
#include "TrafficLights/TLHeadOrientation.h"
#include "UObject/ConstructorHelpers.h"

static TMap<FString, TSoftObjectPtr<UStaticMesh>> GMeshCache;

static const TMap<FString, FString> GKeyToPath = {
    { TEXT("Vertical_WithVisor"),
      TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
           "SM_TrafficLights_Black_Module_01.SM_TrafficLights_Black_Module_01") },
    { TEXT("Vertical_NoVisor"),
      TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
           "SM_TrafficLights_Black_Module_02.SM_TrafficLights_Black_Module_02") },
    { TEXT("Horizontal_WithVisor"),
      TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
           "SM_TrafficLights_Horizontal_Black_Module_01."
           "SM_TrafficLights_Horizontal_Black_Module_01") },
    { TEXT("Horizontal_NoVisor"),
      TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
           "SM_TrafficLights_Horizontal_Black_Module_02."
           "SM_TrafficLights_Horizontal_Black_Module_02") }
};

UStaticMesh* FModuleMeshFactory::GetMeshForModule(const FTLHead& Head, const FTLModule& Module)
{
    const FString Key {FString::Printf(
        TEXT("%s_%s"),
        Head.Orientation == ETLHeadOrientation::Vertical
            ? TEXT("Vertical")
            : TEXT("Horizontal"),
        Module.bHasVisor ? TEXT("WithVisor") : TEXT("NoVisor")
    )};

    TSoftObjectPtr<UStaticMesh>* CachedPtr = GMeshCache.Find(Key);
    if (!CachedPtr)
    {
        const FString* PathPtr = GKeyToPath.Find(Key);
        if (!PathPtr)
        {
            UE_LOG(LogTemp, Error,
                TEXT("ModuleMeshFactory: unknown key '%s'"), *Key);
            return nullptr;
        }

        FSoftObjectPath SoftPath(*PathPtr);
        TSoftObjectPtr<UStaticMesh> SoftPtr{ SoftPath };
        GMeshCache.Add(Key, SoftPtr);
        CachedPtr = GMeshCache.Find(Key);
    }

    UStaticMesh* Mesh {CachedPtr->LoadSynchronous()};
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ModuleMeshFactory: could not be loaded for key '%s' (%s)"),
            *Key, *CachedPtr->ToString());
    }
    return Mesh;
}
