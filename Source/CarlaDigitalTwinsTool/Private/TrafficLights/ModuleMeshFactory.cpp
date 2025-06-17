#include "TrafficLights/ModuleMeshFactory.h"
#include "UObject/SoftObjectPath.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"
#include "TrafficLights/TLHeadOrientation.h"
#include "TrafficLights/TLHeadStyle.h"
#include "UObject/ConstructorHelpers.h"

static TMap<FString, TSoftObjectPtr<UStaticMesh>> GMeshCache;

static const TMap<FString, FString> GKeyToPath = {
    // Vertical With Visor
    { "Vertical_WithVisor_Black",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Black_Module_01.SM_TrafficLights_Black_Module_01" },
    { "Vertical_WithVisor_Color",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Module_01.SM_TrafficLights_Module_01" },

    // Vertical No Visor
    { "Vertical_NoVisor_Black",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Black_Module_02.SM_TrafficLights_Black_Module_02" },
    { "Vertical_NoVisor_Color",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Module_02.SM_TrafficLights_Module_02" },

    // Horizontal With Visor
    { "Horizontal_WithVisor_Black",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Horizontal_Black_Module_01."
      "SM_TrafficLights_Horizontal_Black_Module_01" },
    { "Horizontal_WithVisor_Color",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Horizontal_Module_01."
      "SM_TrafficLights_Horizontal_Module_01" },

    // Horizontal No Visor
    { "Horizontal_NoVisor_Black",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Horizontal_Black_Module_02."
      "SM_TrafficLights_Horizontal_Black_Module_02" },
    { "Horizontal_NoVisor_Color",
      "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "SM_TrafficLights_Horizontal_Module_02."
      "SM_TrafficLights_Horizontal_Module_02" },

    // Crosswalk Module
    { "Crosswalk_Black",
        "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Black_Crosswalk_03.SM_TrafficLights_Black_Crosswalk_03" },
    {"Crosswalk_Black",
        "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Black_Crosswalk_04.SM_TrafficLights_Black_Crosswalk_04"},
    { "Crosswalk_01",
        "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Crosswalk_01.SM_TrafficLights_Crosswalk_01" },
    { "Crosswalk_02",
        "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Crosswalk_02.SM_TrafficLights_Crosswalk_02" },
    { "Crosswalk_03",
        "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Crosswalk_03.SM_TrafficLights_Crosswalk_03" },
    { "Crosswalk_04",
        "/Game/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Crosswalk_04.SM_TrafficLights_Crosswalk_04" },
};

UStaticMesh* FModuleMeshFactory::GetMeshForModule(const FTLHead& Head, const FTLModule& Module)
{
    const bool bUseBlack {Head.Style == ETLHeadStyle::European};
    const FString StyleKey {bUseBlack ? TEXT("Black") : TEXT("Color")};

    const FString Orient {Head.Orientation == ETLHeadOrientation::Vertical ? TEXT("Vertical") : TEXT("Horizontal")};
    const FString Visor  {Module.bHasVisor ? TEXT("WithVisor") : TEXT("NoVisor")};

    const FString Key = Orient + TEXT("_") + Visor + TEXT("_") + StyleKey;

    TSoftObjectPtr<UStaticMesh>* CachedPtr = GMeshCache.Find(Key);
    if (!CachedPtr)
    {
        const FString* PathPtr = GKeyToPath.Find(Key);
        if (!PathPtr)
        {
            UE_LOG(LogTemp, Error,
                TEXT("ModuleMeshFactory: clave desconocida '%s'"), *Key);
            return nullptr;
        }
        TSoftObjectPtr<UStaticMesh> SoftPtr{ FSoftObjectPath(*PathPtr) };
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
