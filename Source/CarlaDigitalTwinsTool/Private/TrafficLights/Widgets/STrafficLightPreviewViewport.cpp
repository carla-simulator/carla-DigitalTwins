#include "TrafficLights/Widgets/STrafficLightPreviewViewport.h"
#include "TrafficLights/MaterialFactory.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TrafficLights/TLHead.h"
#include "UObject/UObjectGlobals.h"


void STrafficLightPreviewViewport::Construct(const FArguments& InArgs)
{
    PreviewScene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues());

    ViewportClient = MakeShareable(new FEditorViewportClient(nullptr, PreviewScene.Get(), nullptr));
    ViewportClient->bSetListenerPosition = false;
    ViewportClient->SetRealtime(true);
    ViewportClient->SetViewLocation(FVector(-300, 0, 150));
    ViewportClient->SetViewRotation(FRotator(-15, 0, 0));
    ViewportClient->SetViewMode(VMI_Lit);
    ViewportClient->SetAllowCinematicControl(true);
    ViewportClient->VisibilityDelegate.BindLambda([]() { return true; });
    ViewportClient->EngineShowFlags.SetGrid(true);

    SAssignNew(ViewportWidget, SViewport)
        .EnableGammaCorrection(false)
        .EnableBlending(true);

    SceneViewport = MakeShareable(new FSceneViewport(ViewportClient.Get(), ViewportWidget));
    ViewportClient->Viewport = SceneViewport.Get();
    ViewportWidget->SetViewportInterface(SceneViewport.ToSharedRef());

    ChildSlot
    [
        ViewportWidget.ToSharedRef()
    ];
}

STrafficLightPreviewViewport::~STrafficLightPreviewViewport()
{
    if (ViewportClient.IsValid())
    {
        FlushRenderingCommands();
        PreviewScene.Reset();
        ViewportClient->Viewport = nullptr;
    }
}

void STrafficLightPreviewViewport::SetHeadStyle(int32 Index, ETLHeadStyle Style)
{
}

void STrafficLightPreviewViewport::AddBackplateMesh(int32 HeadIndex)
{
    if (!HeadMeshComponents.IsValidIndex(HeadIndex))
    {
        return;
    }

    if (BackplateMeshComponents.IsValidIndex(HeadIndex) &&
        BackplateMeshComponents[HeadIndex] != nullptr)
    {
        return;
    }

    const FVector HeadLoc = HeadMeshComponents[HeadIndex]->GetComponentLocation();
    const FVector BackplateLoc = HeadLoc + FVector(10.f, 0.f, 0.f);

    UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(
        PreviewScene->GetWorld()->GetCurrentLevel(), NAME_None, RF_Transient
    );
    Comp->SetStaticMesh(
        Cast<UStaticMesh>(StaticLoadObject(
            UStaticMesh::StaticClass(), nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube")
        ))
    );
    Comp->SetWorldScale3D(FVector(0.1f, 1.2f, 1.2f));
    Comp->SetWorldLocation(BackplateLoc);

    Comp->RegisterComponentWithWorld(PreviewScene->GetWorld());
    Comp->AttachToComponent(
        HeadMeshComponents[HeadIndex],
        FAttachmentTransformRules::KeepWorldTransform
    );

    if (BackplateMeshComponents.Num() <= HeadIndex)
    {
        BackplateMeshComponents.SetNum(HeadMeshComponents.Num());
    }
    BackplateMeshComponents[HeadIndex] = Comp;
}

void STrafficLightPreviewViewport::RemoveBackplateMesh(int32 HeadIndex)
{
    if (!BackplateMeshComponents.IsValidIndex(HeadIndex))
        return;

    if (UStaticMeshComponent* Comp = BackplateMeshComponents[HeadIndex])
    {
        Comp->DestroyComponent();
        BackplateMeshComponents[HeadIndex] = nullptr;
    }
}

FLinearColor STrafficLightPreviewViewport::InitialColorFor(ETLHeadStyle Style) const
{
    switch (Style)
    {
        case ETLHeadStyle::NorthAmerican:
            return FLinearColor::Black;
        case ETLHeadStyle::European:
            return FLinearColor::Blue;
        case ETLHeadStyle::Asian:
            return FLinearColor::White;
        case ETLHeadStyle::Custom:
        default:
            return FLinearColor(1,0,1);
    }
}

UStaticMeshComponent* STrafficLightPreviewViewport::AddModuleMesh(const FTLHead& Head, FTLModule& ModuleData)
{
    const FTransform ModuleWorldTransform {ModuleData.Transform * Head.Transform * ModuleData.Offset};

    UWorld* World {PreviewScene->GetWorld()};
    UObject* LevelOuter {World->PersistentLevel};
    UStaticMeshComponent* Comp {NewObject<UStaticMeshComponent>(LevelOuter, NAME_None, RF_Transient)};
    if (!Comp)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create StaticMeshComponent for module"));
        return nullptr;
    }
    PreviewScene->AddComponent(Comp, ModuleWorldTransform);

    Comp->SetStaticMesh(ModuleData.ModuleMesh);
    if (UMaterialInterface* BodyMat = FMaterialFactory::GetModuleBodyMaterial(Head, ModuleData))
    {
        UMaterialInstanceDynamic* BodyMID = UMaterialInstanceDynamic::Create(BodyMat, Comp);
        Comp->SetMaterial(0, BodyMID);
        ModuleData.BodyMID = BodyMID;
    }
    if (UMaterialInterface* LightMat = FMaterialFactory::GetLightMaterial(ModuleData.LightType))
    {
        UMaterialInstanceDynamic* LightMID = UMaterialInstanceDynamic::Create(LightMat, Comp);
        LightMID->SetScalarParameterValue(TEXT("Emmisive Intensity"), ModuleData.EmissiveIntensity);
        LightMID->SetVectorParameterValue(TEXT("Emissive Color"), ModuleData.EmissiveColor);
        Comp->SetMaterial(1, LightMID);
        ModuleData.LightMID = LightMID;
    }

    ModuleMeshComponents.Add(Comp);

    return Comp;
}

void STrafficLightPreviewViewport::ClearModuleMeshes()
{
    for (UStaticMeshComponent* Comp : ModuleMeshComponents)
    {
        if (Comp)
        {
            PreviewScene->RemoveComponent(Comp);
            Comp->DestroyComponent();
        }
    }
    ModuleMeshComponents.Empty();
}

void STrafficLightPreviewViewport::RecreateModuleMeshesForHead(FTLHead& Head)
{
    ClearModuleMeshes();

    for (FTLModule& ModuleData : Head.Modules)
    {
        ModuleData.ModuleMeshComponent = AddModuleMesh(Head, ModuleData);
    }
}
