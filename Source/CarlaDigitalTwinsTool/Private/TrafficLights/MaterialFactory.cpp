#include "TrafficLights/MaterialFactory.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"

static TMap<FString, TSoftObjectPtr<UMaterialInterface>> GModuleBodyMatCache;
static TMap<ETLLightType, TSoftObjectPtr<UMaterialInterface>> GLightMatCache;

static const TMap<FString, FString> GBodyKeyToPath = {
    { "Module1_Black",
      "/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "M_TrafficLight_ModuleBlack_01.M_TrafficLight_ModuleBlack_01" },
    { "Module1_Color",
      "/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "M_TrafficLight_Module_01.M_TrafficLight_Module_01" },

    { "Module2_Black",
      "/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "M_TrafficLight_ModuleBlack_02.M_TrafficLight_ModuleBlack_02" },
    { "Module2_Color",
      "/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/"
      "M_TrafficLight_Module_02.M_TrafficLight_Module_02" },
};

static const TMap<ETLLightType, FString> GLightTypeToPath = {
    { ETLLightType::Red,    TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/M_TrafficLights_Inst.M_TrafficLights_Inst") },
    { ETLLightType::Yellow, TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/M_TrafficLights_Inst.M_TrafficLights_Inst") },
    { ETLLightType::Green,  TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/M_TrafficLights_Inst.M_TrafficLights_Inst") }
};

UMaterialInterface* FMaterialFactory::GetModuleBodyMaterial(
    const FTLHead& Head,
    const FTLModule& Module)
{
    const FString ModuleKey = Module.bHasVisor ? TEXT("Module1") : TEXT("Module2");

    const bool bUseBlack = (Head.Style == ETLHeadStyle::European);
    const FString StyleKey = bUseBlack ? TEXT("Black") : TEXT("Color");

    const FString Key = ModuleKey + TEXT("_") + StyleKey;

    TSoftObjectPtr<UMaterialInterface>* Ptr = GModuleBodyMatCache.Find(Key);
    if (!Ptr)
    {
        const FString* PathPtr = GBodyKeyToPath.Find(Key);
        if (!PathPtr)
        {
            UE_LOG(LogTemp, Error,
                TEXT("MaterialFactory: Unknown key '%s'"), *Key);
            return nullptr;
        }
        TSoftObjectPtr<UMaterialInterface> SoftPtr{ FSoftObjectPath(*PathPtr) };
        GModuleBodyMatCache.Add(Key, SoftPtr);
        Ptr = GModuleBodyMatCache.Find(Key);
    }

    UMaterialInterface* Mat = Ptr->LoadSynchronous();
    if (!Mat)
    {
        UE_LOG(LogTemp, Error,
            TEXT("MaterialFactory: failed to load the material for '%d'"), *Key);
    }
    return Mat;
}

UMaterialInterface* FMaterialFactory::GetLightMaterial(ETLLightType LightType)
{
    TSoftObjectPtr<UMaterialInterface>* Ptr = GLightMatCache.Find(LightType);
    if (!Ptr)
    {
        const FString* PathPtr = GLightTypeToPath.Find(LightType);
        if (!PathPtr)
        {
            UE_LOG(LogTemp, Error,
                TEXT("MaterialFactory: Unknown LightType '%d'"), (int)LightType);
            return nullptr;
        }
        TSoftObjectPtr<UMaterialInterface> SoftPtr{ FSoftObjectPath(*PathPtr) };
        GLightMatCache.Add(LightType, SoftPtr);
        Ptr = GLightMatCache.Find(LightType);
    }

    UMaterialInterface* Mat = Ptr->LoadSynchronous();
    if (!Mat)
    {
        UE_LOG(LogTemp, Error,
            TEXT("MaterialFactory: failed to load the material for '%d'"), (int)LightType);
    }
    return Mat;
}
