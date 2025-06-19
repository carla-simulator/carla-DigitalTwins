// Copyright (c) 2021 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Commandlet/GenerateRoadRender.h"
#include "Generation/OpenDriveToMap.h"
#if WITH_EDITOR
#include "FileHelpers.h"
#endif
#include "UObject/ConstructorHelpers.h"

#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "UObject/SoftObjectPath.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "RenderUtils.h"
#include "RHICommandList.h"
#include "HighResScreenshot.h"
#include "Runtime/ImageWriteQueue/Public/ImagePixelData.h"
#include "Runtime/ImageWriteQueue/Public/ImageWriteTask.h"
#include "Runtime/ImageWriteQueue/Public/ImageWriteQueue.h"
#include "Online/CustomFileDownloader.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogCarlaToolsMapGenerateRoadRender);


UGenerateRoadRender::UGenerateRoadRender()
{
#if WITH_EDITOR
  ConstructorHelpers::FClassFinder<UOpenDriveToMap> OpenDrivelassFinder(TEXT("/CarlaDigitalTwinsTool/digitalTwins/BP_OpenDriveToMap"));
  OpenDriveClass = OpenDrivelassFinder.Class;
#endif
}

UGenerateRoadRender::UGenerateRoadRender(const FObjectInitializer& Initializer)
  : Super(Initializer)
{
#if WITH_EDITOR
  ConstructorHelpers::FClassFinder<UOpenDriveToMap> OpenDrivelassFinder(TEXT("/CarlaDigitalTwinsTool/digitalTwins/BP_OpenDriveToMap"));
  OpenDriveClass = OpenDrivelassFinder.Class;
#endif
}

#if WITH_EDITORONLY_DATA

int32 UGenerateRoadRender::Main(const FString &Params)
{
	FString MapPath;
	FParse::Value(*Params, TEXT("MapPath="), MapPath);

	auto World = UEditorLoadingAndSavingUtils::LoadMap(MapPath);

	FBox Bounds;
	TArray<AActor*> HiddenActors;
	{
		TSet<AActor*> MeshActorSet;
		{
			TArray<AActor*> MeshActors;
			UGameplayStatics::GetAllActorsOfClass(World, UStaticMesh::StaticClass(), MeshActors);
			for (auto& MeshActor : MeshActors)
			{
				auto SMA = Cast<AStaticMeshActor>(MeshActor);
				if (!SMA)
					continue;
				auto SMC = SMA->GetStaticMeshComponent();
				if (!SMC)
					continue;
				auto SM = SMC->GetStaticMesh();
				if (!SM)
					continue;
				Bounds = Bounds.Overlap(SM->GetBoundingBox());
				MeshActorSet.Add(MeshActor);
			}
		}
		{
			UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), HiddenActors);
			for (int32 i = 0; i != HiddenActors.Num();)
			{
				auto Actor = HiddenActors[i];
				if (MeshActorSet.Contains(Actor) || Actor->IsHidden())
				{
					HiddenActors.RemoveAtSwap(i, EAllowShrinking::Yes);
					continue;
				}
				Actor->SetActorHiddenInGame(true);
				++i;
			}
		}
	}

	auto Min = Bounds.Min;
	auto Max = Bounds.Max;
	auto Center = (Max + Min) / 2.0F;
	auto Extent = (Max - Min) / 2.0F;
	auto OrthoWidth = std::max(Extent.X, Extent.Y) * 2.0F;
	auto Location = FVector(Center.X, Center.Y, Max.Z);
	auto Rotation = FRotator(-90, 0, 0);

	auto RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->AddToRoot();
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->InitAutoFormat(Extent.X, Extent.Y);
	RenderTarget->UpdateResourceImmediate(true);

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActorSpawnParameters.Name = TEXT("camera");
	auto Camera = World->SpawnActor<ASceneCapture2D>(
		ASceneCapture2D::StaticClass(),
		ActorSpawnParameters);
	auto SCC2D = Camera->GetCaptureComponent2D();
	SCC2D->ProjectionType = ECameraProjectionMode::Orthographic;
	SCC2D->OrthoWidth = OrthoWidth;
	SCC2D->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SCC2D->bCaptureEveryFrame = false;
	SCC2D->bCaptureOnMovement = false;
	SCC2D->TextureTarget = RenderTarget;

	Camera->SetActorLocation(Location);
	Camera->SetActorRotation(Rotation);

	SCC2D->CaptureScene();
	FlushRenderingCommands();
	TArray<FColor> Pixels;
	ENQUEUE_RENDER_COMMAND(GetRoadRenderResult)([&](auto& RHICommandList)
	{
		auto Resource = RenderTarget->GetRenderTargetResource();
		Resource->ReadPixels(Pixels);
	});
	FlushRenderingCommands();

	auto Shape = FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY);
	TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
	auto PixelData = MakeUnique<TImagePixelData<FColor>>(Shape);
	PixelData->Pixels = Pixels;
	ImageTask->PixelData = MoveTemp(PixelData);
	ImageTask->Filename = "D:\\Feina\\Test.png";
	ImageTask->Format = EImageFormat::PNG;
	ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
	ImageTask->bOverwriteFile = true;
	ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));

	auto& HighResScreenshotConfig = GetHighResScreenshotConfig();
	HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));

	for (auto& HiddenActor : HiddenActors)
		HiddenActor->SetActorHiddenInGame(false);

	return 0;
}

#endif
