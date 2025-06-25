#include "TrafficLights/Widgets/TLWTrafficLightPreviewViewport.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLLightTypeDataTable.h"
#include "TrafficLights/TLMaterialFactory.h"
#include "TrafficLights/TLMeshFactory.h"
#include "UObject/UObjectGlobals.h"

void STrafficLightPreviewViewport::Construct(const FArguments& InArgs)
{
	LightTypesTable = FTLMeshFactory::GetLightTypeMeshTable();
	ModulesTable = FTLMeshFactory::GetModuleMeshTable();
	PolesTable = FTLMeshFactory::GetPoleMeshTable();

	PreviewScene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues());

	ViewportClient = MakeShareable(new FEditorViewportClient(nullptr, PreviewScene.Get(), nullptr));
	ViewportClient->bSetListenerPosition = false;
	ViewportClient->SetRealtime(false);
	ViewportClient->SetViewLocation(FVector(-300, 0, 150));
	ViewportClient->SetViewRotation(FRotator(0, 0, 0));
	ViewportClient->SetViewMode(VMI_Lit);
	ViewportClient->SetAllowCinematicControl(true);
	ViewportClient->VisibilityDelegate.BindLambda([]() { return true; });
	ViewportClient->EngineShowFlags.SetGrid(true);

	SAssignNew(ViewportWidget, SViewport).EnableGammaCorrection(false).EnableBlending(true);

	SceneViewport = MakeShareable(new FSceneViewport(ViewportClient.Get(), ViewportWidget));
	ViewportClient->Viewport = SceneViewport.Get();
	ViewportWidget->SetViewportInterface(SceneViewport.ToSharedRef());

	ChildSlot[ViewportWidget.ToSharedRef()];
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

void STrafficLightPreviewViewport::SetHeadStyle(int32 Index, ETLStyle Style)
{
}

FVector2D STrafficLightPreviewViewport::GetAtlasCoordsForLightType(ETLLightType LightType) const
{
	if (LightTypesTable == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("LightTypesTable is not set"));
		return FVector2D::ZeroVector;
	}
	const UEnum* EnumPtr = StaticEnum<ETLLightType>();
	if (!EnumPtr)
	{
		return FVector2D::ZeroVector;
	}

	const FString EnumName = EnumPtr->GetNameStringByValue(static_cast<int64>(LightType));
	const FName RowName(*EnumName);

	if (const FTLLightTypeRow* Row = LightTypesTable->FindRow<FTLLightTypeRow>(RowName, TEXT("GetAtlasCoordsForLightType")))
	{
		return Row->AtlasCoords;
	}

	return FVector2D::ZeroVector;
}

UStaticMeshComponent* STrafficLightPreviewViewport::AddModuleMesh(const FTLHead& Head, FTLModule& ModuleData)
{
	const FTransform ModuleWorldTransform{ModuleData.Transform * Head.Transform * ModuleData.Offset};

	UWorld* World{PreviewScene->GetWorld()};
	UObject* LevelOuter{World->PersistentLevel};
	UStaticMeshComponent* Comp{NewObject<UStaticMeshComponent>(LevelOuter, NAME_None, RF_Transient)};
	if (!Comp)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create StaticMeshComponent for module"));
		return nullptr;
	}
	PreviewScene->AddComponent(Comp, ModuleWorldTransform);
	Comp->SetStaticMesh(ModuleData.ModuleMesh);

	if (ModuleData.LightMID == nullptr)
	{
		ModuleData.LightMID = FMaterialFactory::GetLightMaterialInstance(Comp);
	}
	UMaterialInstanceDynamic* LightMID = ModuleData.LightMID;
	if (LightMID)
	{
		LightMID->SetScalarParameterValue(TEXT("Emmisive Intensity"), ModuleData.EmissiveIntensity);
		LightMID->SetVectorParameterValue(TEXT("Emissive Color"), ModuleData.EmissiveColor);
		LightMID->SetScalarParameterValue(TEXT("Offset U"), static_cast<float>(ModuleData.U));
		LightMID->SetScalarParameterValue(TEXT("Offset Y"), static_cast<float>(ModuleData.V));
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
			if (Comp->IsRegistered())
			{
				Comp->UnregisterComponent();
			}
			Comp->DestroyComponent();
		}
	}
	ModuleMeshComponents.Empty();
}

void STrafficLightPreviewViewport::Rebuild(const TArray<FTLPole*>& Poles)
{
	ClearModuleMeshes();
	for (FTLPole* Pole : Poles)
	{
		for (FTLHead& Head : Pole->Heads)
		{
			for (FTLModule& Module : Head.Modules)
			{
				Module.ModuleMeshComponent = AddModuleMesh(Head, Module);
			}
		}
	}
	if (SceneViewport.IsValid())
	{
		SceneViewport->Invalidate();
	}
	if (ModuleMeshComponents.Num() > 0)
	{
		ResetFrame(ModuleMeshComponents[0]);
	}
}

void STrafficLightPreviewViewport::ResetFrame(const UStaticMeshComponent* Comp)
{
	if (!Comp)
	{
		UE_LOG(LogTemp, Error, TEXT("ResetFrame: Invalid Comp"));
		return;
	}
	if (!ViewportClient.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("ResetFrame: Invalid ViewportClient"));
		return;
	}

	FBox Box(EForceInit::ForceInit);
	Box += Comp->Bounds.GetBox();

	const FVector Center{Box.GetCenter()};
	const float Radius{Box.GetExtent().GetMax()};
	const float Distance{Radius * -10.0f};
	const FVector Forward{FVector::ForwardVector.Rotation().RotateVector(FVector(0, 1, 0))};
	const FVector Up{FVector::UpVector};
	const FVector CamPos{Center - Forward * Distance + Up * (Radius * 0.5f)};
	const FRotator CamRot(0.0f, -90.0f, 0.0f);

	ViewportClient->SetViewLocation(CamPos);
	ViewportClient->SetViewRotation(CamRot);
}
