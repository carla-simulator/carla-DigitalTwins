// Copyright (c) 2023 Computer Vision Center (CVC) at the Universitat Autonoma de Barcelona (UAB). This work is licensed under the terms of the MIT license. For a copy, see <https://opensource.org/licenses/MIT>.

#include "Generation/OpenDriveToMap.h"
#if ENGINE_MAJOR_VERSION < 5
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#else
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#endif
#include "Misc/FileHelper.h"
#include "Engine/LevelBounds.h"
#include "Engine/SceneCapture2D.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "StaticMeshAttributes.h"
#include "FileHelpers.h"
#include "Online/CustomFileDownloader.h"
#if ENGINE_MAJOR_VERSION < 5
#include "Engine/Classes/Interfaces/Interface_CollisionDataProvider.h"
#endif
#include "Engine/TriggerBox.h"
#include "Engine/AssetManager.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#if ENGINE_MAJOR_VERSION < 5
#include "PhysicsCore/Public/BodySetupEnums.h"
#endif
#include "PhysicsEngine/BodySetup.h"
#include "RawMesh.h"
#if ENGINE_MAJOR_VERSION < 5
#include "AssetRegistryModule.h"
#endif
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "MeshDescription.h"
#include "EditorLevelLibrary.h"
#include "ProceduralMeshConversion.h"
#include "EditorLevelLibrary.h"
#if ENGINE_MAJOR_VERSION > 4
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Editor/Transactor.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#endif
#include "ContentBrowserModule.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Math/Vector.h"
#include "GameFramework/Actor.h"
#include "EditorLevelUtils.h"
#include "Actor/ProceduralCustomMesh.h"
#include "Actor/LargeMapManager.h"
#include "CarlaDigitalTwinsTool.h"
#include "Kismet/GameplayStatics.h"
#include "Generation/MapGenFunctionLibrary.h"
#include "Carla/Geom/Simplification.h"
#include "Carla/Geom/Deformation.h"
#include "Generation/MapGenFunctionLibrary.h"
#include "BlueprintUtil/BlueprintUtilFunctions.h"
#include "Carla/OpenDrive/OpenDriveParser.h"
#include "Carla/RPC/String.h"

#include "DrawDebugHelpers.h"
#include "Paths/GenerationPathsHelper.h"

#if WITH_EDITOR

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
#include "Engine/Texture2D.h"
// #include "Utils/GoogleStreetViewManager.h"
#include "Utils/GeometryImporter.h"
struct FTerrainMeshData
{
  int32 MeshIndex;
  FVector2D Offset;
  TArray<FVector> Vertices;
  TArray<int32> Triangles;
  TArray<FVector2D> UVs;
  TArray<FVector> Normals;
  TArray<FProcMeshTangent> Tangents;
};

UOpenDriveToMap::UOpenDriveToMap()
{
  AddToRoot();
  bRoadsFinished = false;
  bHasStarted = false;
  bMapLoaded = false;
}

UOpenDriveToMap::~UOpenDriveToMap()
{

}

FString LaneTypeToFString(carla::road::Lane::LaneType LaneType)
{
  switch (LaneType)
  {
  case carla::road::Lane::LaneType::Driving:
    return FString("Driving");
    break;
  case carla::road::Lane::LaneType::Stop:
    return FString("Stop");
    break;
  case carla::road::Lane::LaneType::Shoulder:
    return FString("Shoulder");
    break;
  case carla::road::Lane::LaneType::Biking:
    return FString("Biking");
    break;
  case carla::road::Lane::LaneType::Sidewalk:
    return FString("Sidewalk");
    break;
  case carla::road::Lane::LaneType::Border:
    return FString("Border");
    break;
  case carla::road::Lane::LaneType::Restricted:
    return FString("Restricted");
    break;
  case carla::road::Lane::LaneType::Parking:
    return FString("Parking");
    break;
  case carla::road::Lane::LaneType::Bidirectional:
    return FString("Bidirectional");
    break;
  case carla::road::Lane::LaneType::Median:
    return FString("Median");
    break;
  case carla::road::Lane::LaneType::Special1:
    return FString("Special1");
    break;
  case carla::road::Lane::LaneType::Special2:
    return FString("Special2");
    break;
  case carla::road::Lane::LaneType::Special3:
    return FString("Special3");
    break;
  case carla::road::Lane::LaneType::RoadWorks:
    return FString("RoadWorks");
    break;
  case carla::road::Lane::LaneType::Tram:
    return FString("Tram");
    break;
  case carla::road::Lane::LaneType::Rail:
    return FString("Rail");
    break;
  case carla::road::Lane::LaneType::Entry:
    return FString("Entry");
    break;
  case carla::road::Lane::LaneType::Exit:
    return FString("Exit");
    break;
  case carla::road::Lane::LaneType::OffRamp:
    return FString("OffRamp");
    break;
  case carla::road::Lane::LaneType::OnRamp:
    return FString("OnRamp");
    break;
  case carla::road::Lane::LaneType::Any:
    return FString("Any");
    break;
  }

  return FString("Empty");
}

void UOpenDriveToMap::ConvertOSMInOpenDrive()
{
  FilePath = UGenerationPathsHelper::GetRawMapDirectoryPath(MapName) + "OpenDrive/" + MapName + ".osm";
  FileDownloader->ConvertOSMInOpenDrive( FilePath , OriginGeoCoordinates.X, OriginGeoCoordinates.Y);
  FilePath.RemoveFromEnd(".osm", ESearchCase::Type::IgnoreCase);
  FilePath += ".xodr";

  DownloadFinished();
  UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
  LoadMap();
}

void UOpenDriveToMap::CreateMap()
{
  if( MapName.IsEmpty() )
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Map Name Is Empty") );
    return;
  }

  if( !Url.IsEmpty() ) {
    if ( !IsValid(FileDownloader) )
    {
      FileDownloader = NewObject<UCustomFileDownloader>();
    }

    FileDownloader->ResultFileName = MapName;
    FileDownloader->Url = Url;

    FileDownloader->DownloadDelegate.BindUObject( this, &UOpenDriveToMap::ConvertOSMInOpenDrive );
    FileDownloader->StartDownload();
  }
  else if(LocalFilePath.EndsWith(".xodr"))
  {
    ImportXODR();
  }
  else if(LocalFilePath.EndsWith(".osm"))
  {
    ImportOSM();
  }
  else
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("URL and Local FilePath are Empty. URL: %s  Local FilePath: %s"), *Url, *LocalFilePath );
  }

}

void UOpenDriveToMap::CreateTerrain(const int NumberOfTerrainX, const int NumberOfTerrainY, const float MeshGridResolution)
{


  if (NumberOfTerrainX <= 0 || NumberOfTerrainY <= 0 || MeshGridResolution <= 0) return;

  float TileSizeX = TileSize / NumberOfTerrainX;
  float TileSizeY = TileSize / NumberOfTerrainY;

  FVector MinBox(MinPosition.X, MaxPosition.Y, 0);

  TArray<FTerrainMeshData> AllMeshData;
  AllMeshData.SetNum((NumberOfTerrainX + 1) * (NumberOfTerrainY + 1));

  ParallelFor((NumberOfTerrainX + 1) * (NumberOfTerrainY + 1), [&](int32 Index)
  {
    int32 x = Index % (NumberOfTerrainX + 1);
    int32 y = Index / (NumberOfTerrainX + 1);

    FVector2D Offset(MinBox.X + x * TileSizeX, MinBox.Y + y * TileSizeY);

    FTerrainMeshData& MeshData = AllMeshData[Index];
    MeshData.MeshIndex = Index;
    MeshData.Offset = Offset;

    const int32 VertsX = MeshGridResolution + 1;
    const int32 VertsY = MeshGridResolution + 1;
    const float StepX = TileSizeX / MeshGridResolution;
    const float StepY = TileSizeY / MeshGridResolution;
    //const float HeightScale = 100.0f; // cm

    TArray<FVector>& Vertices = MeshData.Vertices;
    TArray<int32>& Triangles = MeshData.Triangles;
    TArray<FVector2D>& UVs = MeshData.UVs;
    TArray<FVector>& Normals = MeshData.Normals;
    TArray<FProcMeshTangent>& Tangents = MeshData.Tangents;

    Vertices.Reserve(VertsX * VertsY);
    UVs.Reserve(VertsX * VertsY);
    Triangles.Reserve((VertsX - 1) * (VertsY - 1) * 6);

    for (int32 iy = 0; iy < VertsY; ++iy)
    {
      for (int32 ix = 0; ix < VertsX; ++ix)
      {
        float X = ix * StepX;
        float Y = iy * StepY;
        float Height = GetHeightForLandscape(FVector(Offset.X + X, Offset.Y + Y, 0));
        Vertices.Add(FVector(X, Y, Height));
        UVs.Add(FVector2D(static_cast<float>(ix) / MeshGridResolution, static_cast<float>(iy) / MeshGridResolution));
      }
    }

    for (int32 iy = 0; iy < VertsY - 1; ++iy)
    {
      for (int32 ix = 0; ix < VertsX - 1; ++ix)
      {
        int32 i0 = ix + iy * VertsX;
        int32 i1 = (ix + 1) + iy * VertsX;
        int32 i2 = ix + (iy + 1) * VertsX;
        int32 i3 = (ix + 1) + (iy + 1) * VertsX;

        Triangles.Add(i0);
        Triangles.Add(i2);
        Triangles.Add(i1);

        Triangles.Add(i3);
        Triangles.Add(i1);
        Triangles.Add(i2);
      }
    }

  });

  // Ahora en GameThread, procesar cada mesh para terminar y spawn actors
  AsyncTask(ENamedThreads::GameThread, [this, AllMeshData = MoveTemp(AllMeshData)]() mutable
  {
    for (const FTerrainMeshData& MeshData : AllMeshData)
    {
      TArray<FVector> Normals;
      TArray<FProcMeshTangent> Tangents;

      UKismetProceduralMeshLibrary::CalculateTangentsForMesh(MeshData.Vertices, MeshData.Triangles, MeshData.UVs, Normals, Tangents);

      FProceduralCustomMesh ProcMeshData;
      ProcMeshData.Vertices = MeshData.Vertices;
      ProcMeshData.Triangles = MeshData.Triangles;
      ProcMeshData.Normals = Normals;
      ProcMeshData.UV0 = MeshData.UVs;

      UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultLandscapeMaterial, MapName);
      UMaterialInstance* DuplicatedLandscapeMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

      UStaticMesh* StaticMesh = UMapGenFunctionLibrary::CreateMesh(ProcMeshData, Tangents, DuplicatedLandscapeMaterial, MapName, "Terrain", FName(*FString::Printf(TEXT("SM_LandscapeMesh_%d%s"), MeshData.MeshIndex, *GetStringForCurrentTile())));

      if (!StaticMesh) continue;

      UWorld* World = GetEditorWorld();

      if (!World) continue;

      AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(MeshData.Offset.X, MeshData.Offset.Y, 0), FRotator::ZeroRotator);
      if (!Actor) continue;

      UStaticMeshComponent* MeshComp = Actor->GetStaticMeshComponent();
      MeshComp->SetStaticMesh(StaticMesh);
      Actor->SetActorLabel(FString::Printf(TEXT("LandscapeActor_%d%s"), MeshData.MeshIndex, *GetStringForCurrentTile()));
      Actor->Tags.Add("LandscapeToMove");
      MeshComp->CastShadow = false;

#if ENGINE_MAJOR_VERSION > 4
      Actor->SetIsSpatiallyLoaded(true);
#endif

      Landscapes.Add(Actor);
    }
  });
}

void UOpenDriveToMap::CreateTerrainMesh(const int MeshIndex, const FVector2D Offset, const int TileSizeX, const int TileSizeY, const float MeshResolution)
{
  // const float GridSectionSize = 100.0f; // In cm
  const float HeightScale = 3.0f;

  UWorld* World = UEditorLevelLibrary::GetEditorWorld();
  // Creation of the procedural mesh
  AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>();
  MeshActor->SetActorLocation(FVector(Offset.X, Offset.Y, 0));
  UStaticMeshComponent* Mesh = MeshActor->GetStaticMeshComponent();

  TArray<FVector> Vertices;
  TArray<int32> Triangles;

  TArray<FVector> Normals;
  TArray<FLinearColor> Colors;
  TArray<FProcMeshTangent> Tangents;
  TArray<FVector2D> UVs;


  int VerticesInLineX = MeshResolution + 1;
  int VerticesInLineY = MeshResolution + 1;
  float SpaceBetweenVerticesX = static_cast<float>(TileSizeX)  / MeshResolution;
  float SpaceBetweenVerticesY = static_cast<float>(TileSizeY)  / MeshResolution;
  static int StaticMeshIndex = 0;
  for( int i = 0; i < VerticesInLineX; i++ )
  {
    float X = (i * SpaceBetweenVerticesX);
    for( int j = 0; j < VerticesInLineY; j++ )
    {
      float Y = (j * SpaceBetweenVerticesY);
      float HeightValue = GetHeightForLandscape( FVector( (Offset.X + X),
                                                          (Offset.Y + Y),
                                                          0));
      Vertices.Add(FVector( X, Y, HeightValue));
      UVs.Add(FVector2D(i, j));
    }
  }

  //// Triangles formation. 2 triangles per section.
  for(int i = 0; i < VerticesInLineX - 1; i++)
  {
    for(int j = 0; j < VerticesInLineY - 1; j++)
    {
      Triangles.Add(   j       + (   i       * VerticesInLineX ) );
      Triangles.Add( ( j + 1 ) + (   i       * VerticesInLineX ) );
      Triangles.Add(   j       + ( ( i + 1 ) * VerticesInLineX ) );

      Triangles.Add( ( j + 1 ) + (   i       * VerticesInLineX ) );
      Triangles.Add( ( j + 1 ) + ( ( i + 1 ) * VerticesInLineX ) );
      Triangles.Add(   j       + ( ( i + 1 ) * VerticesInLineX ) );
    }
  }

  UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
    Vertices,
    Triangles,
    UVs,
    Normals,
    Tangents
  );

  FProceduralCustomMesh MeshData;
  MeshData.Vertices = Vertices;
  MeshData.Triangles = Triangles;
  MeshData.Normals = Normals;
  MeshData.UV0 = UVs;
    
  UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultLandscapeMaterial, MapName);
  UMaterialInstance* DuplicatedLandscapeMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

  UStaticMesh* MeshToSet = UMapGenFunctionLibrary::CreateMesh(MeshData,  Tangents, DuplicatedLandscapeMaterial, MapName, "Terrain", FName(TEXT("SM_LandscapeMesh" + FString::FromInt(StaticMeshIndex) + GetStringForCurrentTile() )));
  Mesh->SetStaticMesh(MeshToSet);
  MeshActor->SetActorLabel("SM_LandscapeActor" + FString::FromInt(StaticMeshIndex) + GetStringForCurrentTile() );
  MeshActor->Tags.Add(FName("LandscapeToMove"));
#if ENGINE_MAJOR_VERSION > 4
  MeshActor->SetIsSpatiallyLoaded(true);
#endif
  Mesh->CastShadow = false;
  Landscapes.Add(MeshActor);
  StaticMeshIndex++;
}

AActor* UOpenDriveToMap::SpawnActorWithCheckNoCollisions(UClass* ActorClassToSpawn, FTransform Transform)
{
  UWorld* World = UEditorLevelLibrary::GetEditorWorld();
  FActorSpawnParameters SpawnParameters;
  SpawnParameters.bNoFail = true;
  SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  // Creation of the procedural mesh
  return World->SpawnActor<AActor>(ActorClassToSpawn, Transform, SpawnParameters);

}
void UOpenDriveToMap::GenerateTileStandalone(){
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::GenerateTileStandalone Function called"));

#if PLATFORM_WINDOWS
  GenerateTile();
#else
  GenerateTile();
#endif
  UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
  UEditorLevelLibrary::SaveCurrentLevel();
}

void UOpenDriveToMap::GenerateTile(){

  if( FilePath.IsEmpty() ){
    UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("UOpenDriveToMap::GenerateTile(): Failed to load %s"), *FilePath );
    return;
  }

  FString FileContent;
  FFileHelper::LoadFileToString(FileContent, *FilePath);
  std::string opendrive_xml = carla::rpc::FromLongFString(FileContent);
  UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("UOpenDriveToMap::GenerateTile() Loading File..... "));
  CarlaMap = carla::opendrive::OpenDriveParser::Load(opendrive_xml);

  if (!CarlaMap.has_value())
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Invalid Map"));
  }
  else
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("Valid Map loaded"));
    MapName = FPaths::GetCleanFilename(FilePath);
    MapName.RemoveFromEnd(".xodr", ESearchCase::Type::IgnoreCase);
    UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("MapName %s"), *MapName);

#if ENGINE_MAJOR_VERSION < 5
    UEditorLevelLibrary::LoadLevel(*BaseLevelName);
    AActor* QueryActor = UGameplayStatics::GetActorOfClass(
                            GEditor->GetEditorWorldContext().World(),
                            ALargeMapManager::StaticClass() );
    if( QueryActor != nullptr ){
      ALargeMapManager* LmManager = Cast<ALargeMapManager>(QueryActor);
      LmManager->GenerateMap_Editor();
      NumTilesInXY  = LmManager->GetNumTilesInXY();
      TileSize = LmManager->GetTileSize();
      Tile0Offset = LmManager->GetTile0Offset();
      UEditorLevelLibrary::SaveCurrentLevel();
      LmManager->GetCarlaMapTile(CurrentTilesInXY);
      FCarlaMapTile& CarlaTile = LmManager->GetCarlaMapTile(CurrentTilesInXY);

      UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("Current Tile is %s"), *( CurrentTilesInXY.ToString() ) );
      UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("NumTilesInXY is %s"), *( NumTilesInXY.ToString() ) );
      UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("TileSize is %f"), ( TileSize ) );
      UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("Tile0Offset is %s"), *( Tile0Offset.ToString() ) );
      UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("Tile Name is %s"), *(CarlaTile.Name) );

      UEditorLevelLibrary::LoadLevel(CarlaTile.Name);
#endif
      MinPosition = FVector(CurrentTilesInXY.X * TileSize, CurrentTilesInXY.Y * -TileSize, 0.0f);
      MaxPosition = FVector((CurrentTilesInXY.X + 1.0f ) * TileSize, (CurrentTilesInXY.Y + 1.0f) * -TileSize, 0.0f);

      GenerateAll(CarlaMap, MinPosition, MaxPosition);
      Landscapes.Empty();
      bHasStarted = true;
      bRoadsFinished = true;
      bMapLoaded = true;
      bTileFinished = false;
#if ENGINE_MAJOR_VERSION < 5
    }else{
      UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Largemapmanager not found ") );
    }
#endif
    UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
    UEditorLevelLibrary::SaveCurrentLevel();

#if ENGINE_MAJOR_VERSION < 5
#if PLATFORM_LINUX
    RemoveFromRoot();
#endif
#endif
  }
}

bool UOpenDriveToMap::GoNextTile(){
  CurrentTilesInXY.X++;
  if( CurrentTilesInXY.X >= NumTilesInXY.X ){
    CurrentTilesInXY.X = 0;
    CurrentTilesInXY.Y++;
    if( CurrentTilesInXY.Y >= NumTilesInXY.Y ){
      return false;
    }
  }
  return true;
}

void UOpenDriveToMap::ReturnToMainLevel(){
  Landscapes.Empty();
  FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false, nullptr);
  UEditorLevelLibrary::LoadLevel(*BaseLevelName);
}

void UOpenDriveToMap::CorrectPositionForAllActorsInCurrentTile(){
  TArray<AActor*> FoundActors;
  UGameplayStatics::GetAllActorsOfClass(UEditorLevelLibrary::GetEditorWorld(), AActor::StaticClass(), FoundActors);
  for( AActor* Current : FoundActors){
    Current->AddActorWorldOffset(-MinPosition, false);
    if( AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Current) ){
      UStaticMesh* StaticMesh = MeshActor->GetStaticMeshComponent()->GetStaticMesh();
      if(StaticMesh)
        StaticMesh->ClearFlags(RF_Standalone);
    }
  }
  CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
  GEngine->PerformGarbageCollectionAndCleanupActors();
}

FString UOpenDriveToMap::GetStringForCurrentTile(){
  return FString("_X_") + FString::FromInt(CurrentTilesInXY.X) + FString("_Y_") + FString::FromInt(CurrentTilesInXY.Y);
}

AActor* UOpenDriveToMap::SpawnActorInEditorWorld(UClass* Class, FVector Location, FRotator Rotation){
  return UEditorLevelLibrary::GetEditorWorld()->SpawnActor<AActor>(Class,
    Location, Rotation);
}

void UOpenDriveToMap::OpenFileDialog()
{
  TArray<FString> OutFileNames;
  void* ParentWindowPtr = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
  IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
  if (DesktopPlatform)
  {
    DesktopPlatform->OpenFileDialog(ParentWindowPtr, "Select xodr file", FPaths::ProjectDir(), FString(""), ".xodr", 1, OutFileNames);
  }
  for(FString& CurrentString : OutFileNames)
  {
    FilePath = CurrentString;
    UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("FileObtained %s"), *CurrentString );
  }
}

UWorld* UOpenDriveToMap::GetEditorWorld()
{
	UUnrealEditorSubsystem* UnrealEditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();

  // Check if the world is valid
  if (UnrealEditorSubsystem)
  {
    return UnrealEditorSubsystem->GetEditorWorld();
  }
  UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Not Editor subsystem found"));
  return nullptr;
}

UWorld* UOpenDriveToMap::GetGameWorld()
{
	UUnrealEditorSubsystem* UnrealEditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();

  // Check if the world is valid
  if (UnrealEditorSubsystem)
  {
    return UnrealEditorSubsystem->GetGameWorld();
  }
  UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Not Editor subsystem found"));
  return nullptr;
}

void UOpenDriveToMap::LoadMap()
{
  if( FilePath.IsEmpty() ){
    return;
  }

  FString FileContent;
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::LoadMap(): File to load %s"), *FilePath );
  FFileHelper::LoadFileToString(FileContent, *FilePath);
  std::string opendrive_xml = carla::rpc::FromLongFString(FileContent);
  CarlaMap = carla::opendrive::OpenDriveParser::Load(opendrive_xml);
  
  if (!CarlaMap.has_value())
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Invalid Map"));
  }
  else
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("Valid Map loaded"));
    MapName = FPaths::GetCleanFilename(FilePath);
    MapName.RemoveFromEnd(".xodr", ESearchCase::Type::IgnoreCase);
    UE_LOG(LogCarlaDigitalTwinsTool, Warning, TEXT("MapName %s"), *MapName);

    AActor* QueryActor = UGameplayStatics::GetActorOfClass(
                                UEditorLevelLibrary::GetEditorWorld(),
                                ALargeMapManager::StaticClass() );
#if ENGINE_MAJOR_VERSION < 5
    if( QueryActor != nullptr )
    {
      
      ALargeMapManager* LargeMapManager = Cast<ALargeMapManager>(QueryActor);
      NumTilesInXY  = LargeMapManager->GetNumTilesInXY();
      TileSize = LargeMapManager->GetTileSize();
      Tile0Offset = LargeMapManager->GetTile0Offset();
      CurrentTilesInXY = FIntVector(0,0,0);
      ULevel* PersistantLevel = UEditorLevelLibrary::GetEditorWorld()->PersistentLevel;
      BaseLevelName = LargeMapManager->LargeMapTilePath + "/" + LargeMapManager->LargeMapName;
      do{
        GenerateTileStandalone();
      }while(GoNextTile());
      ReturnToMainLevel();
      
    }
#else
    do{
      GenerateTileStandalone();
    }while(GoNextTile());
    RemoveFromRoot();
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
      FString CurrentMapName = World->GetMapName();
      CurrentMapName.RemoveFromStart(World->StreamingLevelsPrefix);
      UGameplayStatics::OpenLevel(World, FName(*CurrentMapName));
      AsyncTask(ENamedThreads::GameThread, [this, World]
          {
              RenderRoadToTexture(World);
          });
    }
#endif

  }
}

TArray<AActor*> UOpenDriveToMap::GenerateMiscActors(float Offset, FVector MinLocation, FVector MaxLocation )
{
  carla::geom::Vector3D CarlaMinLocation(MinLocation.X / 100, MinLocation.Y / 100, MinLocation.Z /100);
  carla::geom::Vector3D CarlaMaxLocation(MaxLocation.X / 100, MaxLocation.Y / 100, MaxLocation.Z /100);

  std::vector<std::pair<carla::geom::Transform, std::string>>
    Locations = CarlaMap->GetTreesTransform(CarlaMinLocation, CarlaMaxLocation, DistanceBetweenTrees, DistanceFromRoadEdge, Offset);
  TArray<AActor*> Returning;
  static int i = 0;
  for (auto& cl : Locations)
  {
    const FVector scale{ 1.0f, 1.0f, 1.0f };
    cl.first.location.z = GetHeight(cl.first.location.x, cl.first.location.y) + 0.3f;
    FTransform NewTransform ( FRotator(cl.first.rotation), FVector(cl.first.location), scale );

    NewTransform = GetSnappedPosition(NewTransform);

    AActor* Spawner = UEditorLevelLibrary::GetEditorWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),
      NewTransform.GetLocation(), NewTransform.Rotator());
    Spawner->Tags.Add(FName("MiscSpawnPosition"));
    Spawner->Tags.Add(FName(cl.second.c_str()));
    Spawner->SetActorLabel("MiscSpawnPosition" + FString::FromInt(i));
#if ENGINE_MAJOR_VERSION > 4
    Spawner->SetIsSpatiallyLoaded(true);
#endif
    ++i;
    Returning.Add(Spawner);
  }
  return Returning;
}

void UOpenDriveToMap::GenerateAll(const boost::optional<carla::road::Map>& ParamCarlaMap,
  FVector MinLocation,
  FVector MaxLocation )
{
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::GenerateAll() Generating Roads..... "));
  GenerateRoadMesh(ParamCarlaMap, MinLocation, MaxLocation);
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::GenerateAll() Generating Lane Marks..... "));
  GenerateLaneMarks(ParamCarlaMap, MinLocation, MaxLocation);
  // GenerateSpawnPoints(ParamCarlaMap, MinLocation, MaxLocation);
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::GenerateAll() Generating Terrain..... "));
  CreateTerrain(5,5, 64);
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::GenerateAll() Generating Tree positions..... "));
  GenerateTreePositions(ParamCarlaMap, MinLocation, MaxLocation);
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("UOpenDriveToMap::GenerateAll() Generating Misc stuff..... "));
  GenerationFinished(MinLocation, MaxLocation);
}

void UOpenDriveToMap::GenerateRoadMesh( const boost::optional<carla::road::Map>& ParamCarlaMap, FVector MinLocation, FVector MaxLocation )
{
  opg_parameters.vertex_distance = 0.5f;
  opg_parameters.vertex_width_resolution = 8.0f;
#if ENGINE_MAJOR_VERSION < 5
  opg_parameters.simplification_percentage = 50.0f;
#else
  opg_parameters.simplification_percentage = 0.0f;
#endif
  double start = FPlatformTime::Seconds();

  carla::geom::Vector3D CarlaMinLocation(MinLocation.X / 100, MinLocation.Y / 100, MinLocation.Z /100);
  carla::geom::Vector3D CarlaMaxLocation(MaxLocation.X / 100, MaxLocation.Y / 100, MaxLocation.Z /100);
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT(" Generating roads between %s  and %s"), *(CarlaMinLocation.ToFVector().ToString()), *(CarlaMaxLocation.ToFVector().ToString()) );
  const auto Meshes = ParamCarlaMap->GenerateOrderedChunkedMeshInLocations(opg_parameters, CarlaMinLocation, CarlaMaxLocation);
  double end = FPlatformTime::Seconds();
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT(" GenerateOrderedChunkedMesh code executed in %f seconds. Simplification percentage is %f"), end - start, opg_parameters.simplification_percentage);

  start = FPlatformTime::Seconds();
  static int index = 0;

  struct FPreparedMeshData
  {
    FProceduralCustomMesh MeshData;
    FVector MeshCentroid;
    carla::road::Lane::LaneType LaneType;
    int32 Index;
  };

  TArray<FPreparedMeshData> PreparedMeshes;
  FCriticalSection Mutex;
  int32 LocalIndex = 0;

  for (const auto& PairMap : Meshes)
  {
    const auto& LaneType = PairMap.first;
    const auto& MeshList = PairMap.second;

    ParallelFor(MeshList.size(), [&](int32 i)
    {
      const auto& Mesh = MeshList[i];
      if (!Mesh->IsValid() || ( Mesh->GetVertices().size() == 0)) 
        return;

      auto& Vertices = Mesh->GetVertices();

      if (LaneType == carla::road::Lane::LaneType::Driving)
      {
        for (auto& Vertex : Vertices)
        {
          FVector FV = Vertex.ToFVector();
          Vertex.z += GetHeight(Vertex.x, Vertex.y, DistanceToLaneBorder(ParamCarlaMap, FV) > 65.0f);
        }
#if ENGINE_MAJOR_VERSION < 5
        carla::geom::Simplification Simplify(0.15);
        Simplify.Simplificate(Mesh);
#endif
      }
      else
      {
        for (auto& Vertex : Vertices)
        {
          Vertex.z += GetHeight(Vertex.x, Vertex.y, false) + 0.15f;
        }
      }

      FVector Centroid(0);
      for (const auto& V : Vertices) Centroid += V.ToFVector();
      Centroid /= Vertices.size();

      for (auto& V : Vertices)
      {
        V.x -= Centroid.X;
        V.y -= Centroid.Y;
        V.z -= Centroid.Z;
      }

      FPreparedMeshData Data;
      Data.MeshData = *Mesh;
      Data.MeshCentroid = Centroid;
      Data.LaneType = LaneType;

      int32 AssignedIndex;
      {
        FScopeLock Lock(&Mutex);
        AssignedIndex = LocalIndex++;
        Data.Index = AssignedIndex;
        PreparedMeshes.Add(Data);
      }
    });
  }

  for (FPreparedMeshData& Entry : PreparedMeshes)
  {
    const FProceduralCustomMesh& Mesh = Entry.MeshData;
    const FVector& Centroid = Entry.MeshCentroid;
    const int32 Index = Entry.Index;
    const carla::road::Lane::LaneType LaneType = Entry.LaneType;


    TArray<FProcMeshTangent> Tangents;
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Entry.MeshData.Vertices, Entry.MeshData.Triangles, Entry.MeshData.UV0, Entry.MeshData.Normals, Tangents);

    AStaticMeshActor* TempActor = UEditorLevelLibrary::GetEditorWorld()->SpawnActor<AStaticMeshActor>();
    UStaticMeshComponent* StaticMeshComponent = TempActor->GetStaticMeshComponent();
    TempActor->SetActorLabel(FString("SM_Lane_") + FString::FromInt(Index));
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    if (LaneType == carla::road::Lane::LaneType::Driving && DefaultRoadMaterial)
    {
      UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultRoadMaterial, MapName);
      UMaterialInstance* DuplicatedRoadMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

      StaticMeshComponent->SetMaterial(0, DuplicatedRoadMaterial);
      StaticMeshComponent->CastShadow = false;
      TempActor->SetActorLabel(FString("SM_DrivingLane_") + FString::FromInt(Index));
    }

    if (LaneType == carla::road::Lane::LaneType::Sidewalk && DefaultSidewalksMaterial)
    {
      UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultRoadMaterial, MapName);
      UMaterialInstance* DuplicatedSidewalkMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

      StaticMeshComponent->SetMaterial(0, DuplicatedSidewalkMaterial);
      TempActor->SetActorLabel(FString("SM_Sidewalk_") + FString::FromInt(Index));
    }

    UStaticMesh* FinalMesh = nullptr;

    if (LaneType == carla::road::Lane::LaneType::Sidewalk)
    {
      UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultSidewalksMaterial, MapName);
      UMaterialInstance* DuplicatedSidewalkMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

      FinalMesh = UMapGenFunctionLibrary::CreateMesh(Entry.MeshData, Tangents, DuplicatedSidewalkMaterial, MapName, "Sidewalk", FName(TEXT("SM_SidewalkMesh" + FString::FromInt(Index) + GetStringForCurrentTile())));
    }
    else if (LaneType == carla::road::Lane::LaneType::Driving)
    {
      UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultRoadMaterial, MapName);
      UMaterialInstance* DuplicatedRoadMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

      FinalMesh = UMapGenFunctionLibrary::CreateMesh(Entry.MeshData, Tangents, DuplicatedRoadMaterial, MapName, "DrivingLane", FName(TEXT("SM_DrivingLaneMesh" + FString::FromInt(Index) + GetStringForCurrentTile())));
    }

    StaticMeshComponent->SetStaticMesh(FinalMesh);
    TempActor->SetActorLocation(Centroid * 100);
    TempActor->Tags.Add(FName("RoadLane"));
    TempActor->SetActorEnableCollision(true);

#if ENGINE_MAJOR_VERSION > 4
    TempActor->SetIsSpatiallyLoaded(true);
#endif
  }

  end = FPlatformTime::Seconds();
  UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("Mesh spawnning and translation code executed in %f seconds."), end - start);
}


void UOpenDriveToMap::GenerateLaneMarks(const boost::optional<carla::road::Map>& ParamCarlaMap, FVector MinLocation, FVector MaxLocation )
{
  opg_parameters.vertex_distance = 0.5f;
  opg_parameters.vertex_width_resolution = 8.0f;
  opg_parameters.simplification_percentage = 15.0f;
  std::vector<std::string> lanemarkinfo;
  carla::geom::Vector3D CarlaMinLocation(MinLocation.X / 100, MinLocation.Y / 100, MinLocation.Z /100);
  carla::geom::Vector3D CarlaMaxLocation(MaxLocation.X / 100, MaxLocation.Y / 100, MaxLocation.Z /100);
  auto MarkingMeshes = ParamCarlaMap->GenerateLineMarkings(opg_parameters, CarlaMinLocation, CarlaMaxLocation, lanemarkinfo);
  TArray<AActor*> LaneMarkerActorList;
  static int meshindex = 0;
  int index = 0;
  for (const auto& Mesh : MarkingMeshes)
  {

    if ( !Mesh->GetVertices().size() )
    {
      index++;
      continue;
    }
    if ( !Mesh->IsValid() ) {
      index++;
      continue;
    }

    FVector MeshCentroid = FVector(0, 0, 0);
    for (auto& Vertex : Mesh->GetVertices())
    {
      FVector VertexFVector = Vertex.ToFVector();
      Vertex.z += GetHeight(Vertex.x, Vertex.y, DistanceToLaneBorder(ParamCarlaMap,VertexFVector) > 65.0f ) + 0.0001f;
      MeshCentroid += Vertex.ToFVector();
    }

    MeshCentroid /= Mesh->GetVertices().size();

    for (auto& Vertex : Mesh->GetVertices())
    {
      Vertex.x -= MeshCentroid.X;
      Vertex.y -= MeshCentroid.Y;
      Vertex.z -= MeshCentroid.Z;
    }

    // TODO: Improve this code
    float MinDistance = 99999999.9f;
    for(auto SpawnedActor : LaneMarkerActorList)
    {
      float VectorDistance = FVector::Distance(MeshCentroid*100, SpawnedActor->GetActorLocation());
      if(VectorDistance < MinDistance)
      {
        MinDistance = VectorDistance;
      }
    }

    if(MinDistance < 250)
    {
      UE_LOG(LogCarlaDigitalTwinsTool, VeryVerbose, TEXT("Skkipped is %f."), MinDistance);
      index++;
      continue;
    }

    AStaticMeshActor* TempActor = UEditorLevelLibrary::GetEditorWorld()->SpawnActor<AStaticMeshActor>();
    UStaticMeshComponent* StaticMeshComponent = TempActor->GetStaticMeshComponent();
    TempActor->SetActorLabel(FString("SM_LaneMark_") + FString::FromInt(meshindex));
    StaticMeshComponent->CastShadow = false;
    if (lanemarkinfo[index].find("yellow") != std::string::npos) {
      if(DefaultLaneMarksYellowMaterial)
      {
        UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultLaneMarksYellowMaterial, MapName);
        UMaterialInstance* DuplicatedLaneMarksYellowMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

        StaticMeshComponent->SetMaterial(0, DuplicatedLaneMarksYellowMaterial);
      }
    }else{
      if(DefaultLaneMarksWhiteMaterial)
      {
        UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultLaneMarksWhiteMaterial, MapName);
        UMaterialInstance* DuplicatedLaneMarksWhiteMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

        StaticMeshComponent->SetMaterial(0, DuplicatedLaneMarksWhiteMaterial);
      }
    }

    const FProceduralCustomMesh MeshData = *Mesh;
    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
      MeshData.Vertices,
      MeshData.Triangles,
      MeshData.UV0,
      Normals,
      Tangents
    );

    UObject* DuplicatedMaterialObject = UBlueprintUtilFunctions::CopyAssetToPlugin(DefaultLandscapeMaterial, MapName);
    UMaterialInstance* DuplicatedLandscapeMaterial = Cast<UMaterialInstance>(DuplicatedMaterialObject);

    UStaticMesh* MeshToSet = UMapGenFunctionLibrary::CreateMesh(MeshData,  Tangents, DuplicatedLandscapeMaterial, MapName, "LaneMark", FName(TEXT("SM_LaneMarkMesh" + FString::FromInt(meshindex) + GetStringForCurrentTile() )));
    StaticMeshComponent->SetStaticMesh(MeshToSet);
    TempActor->SetActorLocation(MeshCentroid * 100);
    TempActor->Tags.Add(*FString(lanemarkinfo[index].c_str()));
    TempActor->Tags.Add(FName("RoadLane"));
#if ENGINE_MAJOR_VERSION > 4
      TempActor->SetIsSpatiallyLoaded(true);
#endif
    LaneMarkerActorList.Add(TempActor);
    index++;
    meshindex++;
    TempActor->SetActorEnableCollision(false);
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  }
}

/*
void UOpenDriveToMap::GenerateSpawnPoints( const boost::optional<carla::road::Map>& ParamCarlaMap, FVector MinLocation, FVector MaxLocation  )
{
  float SpawnersHeight = 300.f;
  const auto Waypoints = ParamCarlaMap.GenerateWaypointsOnRoadEntries();
  TArray<AActor*> ActorsToMove;
  for (const auto &Wp : Waypoints)
  {
    const FTransform Trans = ParamCarlaMap.ComputeTransform(Wp);
    if( Trans.GetLocation().X >= MinLocation.X && Trans.GetLocation().Y >= MinLocation.Y &&
        Trans.GetLocation().X <= MaxLocation.X && Trans.GetLocation().Y <= MaxLocation.Y)
    {
      AVehicleSpawnPoint *Spawner = UEditorLevelLibrary::GetEditorWorld()->SpawnActor<AVehicleSpawnPoint>();
      Spawner->SetActorRotation(Trans.GetRotation());
      Spawner->SetActorLocation(Trans.GetTranslation() + FVector(0.f, 0.f, SpawnersHeight));
      ActorsToMove.Add(Spawner);
    }
  }
}
  */

void UOpenDriveToMap::GenerateTreePositions( const boost::optional<carla::road::Map>& ParamCarlaMap, FVector MinLocation, FVector MaxLocation  )
{
  carla::geom::Vector3D CarlaMinLocation(MinLocation.X / 100, MinLocation.Y / 100, MinLocation.Z /100);
  carla::geom::Vector3D CarlaMaxLocation(MaxLocation.X / 100, MaxLocation.Y / 100, MaxLocation.Z /100);

  std::vector<std::pair<carla::geom::Transform, std::string>> Locations =
    ParamCarlaMap->GetTreesTransform(CarlaMinLocation, CarlaMaxLocation,DistanceBetweenTrees, DistanceFromRoadEdge );
  int i = 0;
  for (auto &cl : Locations)
  {
    const FVector scale{ 1.0f, 1.0f, 1.0f };
    cl.first.location.z  = GetHeight(cl.first.location.x, cl.first.location.y) + 0.3f;
    FTransform NewTransform ( FRotator(cl.first.rotation), FVector(cl.first.location), scale );
    NewTransform = GetSnappedPosition(NewTransform);

    AActor* Spawner = UEditorLevelLibrary::GetEditorWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),
      NewTransform.GetLocation(), NewTransform.Rotator());

    Spawner->Tags.Add(FName("TreeSpawnPosition"));
    Spawner->Tags.Add(FName(cl.second.c_str()));
    Spawner->SetActorLabel("TreeSpawnPosition" + FString::FromInt(i) + GetStringForCurrentTile() );
#if ENGINE_MAJOR_VERSION > 4
    Spawner->SetIsSpatiallyLoaded(true);
#endif
    ++i;
  }
}

float UOpenDriveToMap::GetHeight(float PosX, float PosY, bool bDrivingLane){
  if( DefaultHeightmap ){
#if ENGINE_MAJOR_VERSION < 5
      auto RawImageData = DefaultHeightmap->PlatformData->Mips[0].BulkData.LockReadOnly();
#else
      auto RawImageData = DefaultHeightmap->GetPlatformData()->Mips[0].BulkData.LockReadOnly();
#endif
    const FColor* FormatedImageData = static_cast<const FColor*>(RawImageData);

    int32 TextureSizeX = DefaultHeightmap->GetSizeX();
    int32 TextureSizeY = DefaultHeightmap->GetSizeY();

    int32 PixelX = ( ( PosX - WorldOriginPosition.X/100) / (WorldEndPosition.X/100 - WorldOriginPosition.X/100) ) * ((float)TextureSizeX);
    int32 PixelY = ( ( PosY - WorldOriginPosition.Y/100) / (WorldEndPosition.Y/100 - WorldOriginPosition.Y/100) ) * ((float)TextureSizeY);

    if( PixelX < 0 ){
      PixelX += TextureSizeX;
    }

    if( PixelY < 0 ){
      PixelY += TextureSizeY;
    }

    if( PixelX > TextureSizeX ){
      PixelX -= TextureSizeX;
    }

    if( PixelY > TextureSizeY ){
      PixelY -= TextureSizeY;
    }

    FColor PixelColor = FormatedImageData[PixelY * TextureSizeX + PixelX];

    //UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("PosX %f PosY %f "), PosX, PosY );
    //UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("WorldOriginPosition %s "), *WorldOriginPosition.ToString() );
    //UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("WorldEndPosition %s "), *WorldEndPosition.ToString() );
    //UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("PixelColor %s "), *WorldEndPosition.ToString() );
    //UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Reading Pixel X: %d Y %d Total Size X %d Y %d"), PixelX, PixelY, TextureSizeX, TextureSizeY );

#if ENGINE_MAJOR_VERSION < 5
    DefaultHeightmap->PlatformData->Mips[0].BulkData.Unlock();
#else
    if (!DefaultHeightmap->GetPlatformData()->Mips[0].BulkData.IsUnlocked())
      DefaultHeightmap->GetPlatformData()->Mips[0].BulkData.Unlock();
#endif

    float LandscapeHeight = ( (PixelColor.R/255.0f ) * ( MaxHeight - MinHeight ) ) + MinHeight;

    if( bDrivingLane ){
      return LandscapeHeight -
        carla::geom::deformation::GetBumpDeformation(PosX,PosY);
    }else{
      return LandscapeHeight;
    }
  }else{
    if( bDrivingLane ){
      return carla::geom::deformation::GetZPosInDeformation(PosX, PosY) +
        (carla::geom::deformation::GetZPosInDeformation(PosX, PosY) * -0.3f) -
        carla::geom::deformation::GetBumpDeformation(PosX,PosY);
    }else{
      return carla::geom::deformation::GetZPosInDeformation(PosX, PosY) + (carla::geom::deformation::GetZPosInDeformation(PosX, PosY) * -0.3f);
    }
  }
}

FTransform UOpenDriveToMap::GetSnappedPosition( FTransform Origin ){
  FTransform ToReturn = Origin;
  FVector Start = Origin.GetLocation() + FVector( 0, 0, 10000);
  FVector End = Origin.GetLocation() - FVector( 0, 0, 10000);
  FHitResult HitResult;
  FCollisionQueryParams CollisionQuery;
  CollisionQuery.bTraceComplex = true;
  FCollisionResponseParams CollisionParams;

  if( UEditorLevelLibrary::GetEditorWorld()->LineTraceSingleByChannel(
    HitResult,
    Start,
    End,
    ECollisionChannel::ECC_WorldStatic,
    CollisionQuery,
    CollisionParams
  ) )
  {
    ToReturn.SetLocation(HitResult.Location);
  }
  return ToReturn;
}

float UOpenDriveToMap::GetHeightForLandscape( FVector Origin ){
  FVector Start = Origin - FVector( 0, 0, 10000000000);
  FVector End = Origin + FVector( 0, 0, 10000000000);
  FHitResult HitResult;
  FCollisionQueryParams CollisionQuery;
  CollisionQuery.AddIgnoredActors(Landscapes);
  FCollisionResponseParams CollisionParams;

  if( UEditorLevelLibrary::GetEditorWorld()->LineTraceSingleByChannel(
    HitResult,
    Start,
    End,
    ECollisionChannel::ECC_WorldStatic,
    CollisionQuery,
    CollisionParams) )
  {
    return HitResult.ImpactPoint.Z - GetHeight(Origin.X * 0.01f, Origin.Y * 0.01f, true) * 100.0f;
  }else{
    return GetHeight(Origin.X * 0.01f, Origin.Y * 0.01f, true) * 100.0f - 1.0f;
  }
  return 0.0f;
}

float UOpenDriveToMap::DistanceToLaneBorder(
  const boost::optional<carla::road::Map>& ParamCarlaMap,
  FVector &location, int32_t lane_type ) const
{
  carla::geom::Location cl(location);
  //wp = GetClosestWaypoint(pos). if distance wp - pos == lane_width --> estas al borde de la carretera
  auto wp = ParamCarlaMap->GetClosestWaypointOnRoad(cl, lane_type);
  if(wp)
  {
    carla::geom::Transform ct = ParamCarlaMap->ComputeTransform(*wp);
    double LaneWidth = ParamCarlaMap->GetLaneWidth(*wp);
    return cl.Distance(ct.location) - LaneWidth;
  }
  return 100000.0f;
}

bool UOpenDriveToMap::IsInRoad(
  const boost::optional<carla::road::Map>& ParamCarlaMap,
  FVector &location) const
{
  int32_t start = static_cast<int32_t>(carla::road::Lane::LaneType::Driving);
  int32_t end = static_cast<int32_t>(carla::road::Lane::LaneType::Sidewalk);
  for( int32_t i = start; i < end; ++i)
  {
    if(ParamCarlaMap->GetWaypoint(location, i))
    {
      return true;
    }
  }
  return false;
}

void UOpenDriveToMap::ImportXODR(){
  IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
  FString MyFileDestination = UGenerationPathsHelper::GetRawMapDirectoryPath(MapName) + MapName + "OpenDrive/" + MapName + ".xodr";

  if(FileManager.CopyFile(  *MyFileDestination, *LocalFilePath,
                              EPlatformFileRead::None,
                              EPlatformFileWrite::None))
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Verbose, TEXT("FilePaths: File Copied!"));
    FilePath = MyFileDestination;
    LoadMap();
  }
  else
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("FilePaths local xodr file not copied: File not Copied!"));
  }
}

void UOpenDriveToMap::ImportOSM(){
  IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
  FString MyFileDestination = UGenerationPathsHelper::GetRawMapDirectoryPath(MapName) + MapName + "OpenDrive/" + MapName + ".osm";

  if(FileManager.CopyFile(  *MyFileDestination, *LocalFilePath,
                              EPlatformFileRead::None,
                              EPlatformFileWrite::None))
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Verbose, TEXT("FilePaths: File Copied!"));
    ConvertOSMInOpenDrive();
  }
  else
  {
    UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("FilePaths local osm file not copied: File not Copied!"));
  }
}

void UOpenDriveToMap::MoveActorsToSubLevels(TArray<AActor*> ActorsToMove)
{
  AActor* QueryActor = UGameplayStatics::GetActorOfClass(
                                UEditorLevelLibrary::GetEditorWorld(),
                                ALargeMapManager::StaticClass() );

  if( QueryActor != nullptr ){
    ALargeMapManager* LmManager = Cast<ALargeMapManager>(QueryActor);
    if( LmManager ){
      UEditorLevelLibrary::SaveCurrentLevel();
      UOpenDriveToMap::MoveActorsToSubLevelWithLargeMap(ActorsToMove, LmManager);
    }
  }
}

void UOpenDriveToMap::MoveActorsToSubLevelWithLargeMap(TArray<AActor*> Actors, ALargeMapManager* LargeMapManager)
{
    TMap<FCarlaMapTile*, TArray<AActor*>> ActorsToMove;
    for (AActor* Actor : Actors)
    {
        if (Actor == nullptr) {
            continue;
        }
        UHierarchicalInstancedStaticMeshComponent* Component
            = Cast<UHierarchicalInstancedStaticMeshComponent>(
                Actor->GetComponentByClass(
                    UHierarchicalInstancedStaticMeshComponent::StaticClass()));
        FVector ActorLocation = Actor->GetActorLocation();
        if (Component)
        {
            ActorLocation = FVector(0);
            for (int32 i = 0; i < Component->GetInstanceCount(); ++i)
            {
                FTransform Transform;
                Component->GetInstanceTransform(i, Transform, true);
                ActorLocation = ActorLocation + Transform.GetTranslation();
            }
            ActorLocation = ActorLocation / Component->GetInstanceCount();
        }
        UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("Actor at location %s"),
            *ActorLocation.ToString());
        FCarlaMapTile* Tile = LargeMapManager->GetCarlaMapTile(ActorLocation);
        if (!Tile)
        {
            UE_LOG(LogCarlaDigitalTwinsTool, Error, TEXT("Error: actor %s in location %s is outside the map"),
                *Actor->GetName(), *ActorLocation.ToString());
            continue;
        }

        if (Component)
        {
            UpdateInstancedMeshCoordinates(Component, Tile->Location);
        }
        else
        {
            UpdateGenericActorCoordinates(Actor, Tile->Location);
        }
        ActorsToMove.FindOrAdd(Tile).Add(Actor);
    }

    for (auto& Element : ActorsToMove)
    {
        FCarlaMapTile* Tile = Element.Key;
        TArray<AActor*> ActorList = Element.Value;
        if (!ActorList.Num())
        {
            continue;
        }

        UWorld* World = UEditorLevelLibrary::GetEditorWorld();
        ULevelStreamingDynamic* StreamingLevel = Tile->StreamingLevel;
        StreamingLevel->bShouldBlockOnLoad = true;
        StreamingLevel->SetShouldBeVisible(true);
        StreamingLevel->SetShouldBeLoaded(true);
        ULevelStreaming* Level =
            UEditorLevelUtils::AddLevelToWorld(
                World, *Tile->Name, ULevelStreamingDynamic::StaticClass(), FTransform());
        int MovedActors = UEditorLevelUtils::MoveActorsToLevel(ActorList, Level, false, false);
        // StreamingLevel->SetShouldBeLoaded(false);
        UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("Moved %d actors"), MovedActors);
        FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false, nullptr);
        UEditorLevelUtils::RemoveLevelFromWorld(Level->GetLoadedLevel());
    }

    GEngine->PerformGarbageCollectionAndCleanupActors();
    FText TransResetText(FText::FromString("Clean up after Move actors to sublevels"));
    if (GEditor->Trans)
    {
#if ENGINE_MAJOR_VERSION < 5
        GEditor->Trans->Reset(TransResetText);
#else
        GEditor->Trans->Reset(TransResetText);
#endif
        GEditor->Cleanse(true, true, TransResetText);
    }
}

void UOpenDriveToMap::UpdateInstancedMeshCoordinates(
    UHierarchicalInstancedStaticMeshComponent* Component, FVector TileOrigin)
{
    TArray<FTransform> NewTransforms;
    for (int32 i = 0; i < Component->GetInstanceCount(); ++i)
    {
        FTransform Transform;
        Component->GetInstanceTransform(i, Transform, true);
        Transform.AddToTranslation(-TileOrigin);
        NewTransforms.Add(Transform);
        UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("New instance location %s"),
            *Transform.GetTranslation().ToString());
    }
    Component->BatchUpdateInstancesTransforms(0, NewTransforms, true, true, true);
}

void UOpenDriveToMap::UpdateGenericActorCoordinates(
    AActor* Actor, FVector TileOrigin)
{
    FVector LocalLocation = Actor->GetActorLocation() - TileOrigin;
    Actor->SetActorLocation(LocalLocation);
    UE_LOG(LogCarlaDigitalTwinsTool, Log, TEXT("%s New location %s"),
        *Actor->GetName(), *LocalLocation.ToString());
}

void UOpenDriveToMap::UnloadWorldPartitionRegion(const FBox& RegionBox)
{
  UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
  if (World)
  {
    UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>();
    if (WorldPartitionSubsystem)
    {
      // Call UnloadRegion with a bounding box
      //WorldPartitionSubsystem->UnloadRegion(World, RegionBox);
    }
  }
}

UTexture2D* UOpenDriveToMap::RenderRoadToTexture(UWorld* World)
{
    const double Limit = 2000000.0;

    FBox Bounds(EForceInit::ForceInitToZero);

    TArray<AActor*> HiddenActors;
    {
        TArray<AActor*> Actors;
        UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
        HiddenActors.Reserve(Actors.Num());
        for (auto& Actor : Actors)
        {
            auto BoundingBox = Actor->GetComponentsBoundingBox();
            Bounds += BoundingBox;
            auto Name = Actor->GetActorLabel();
            if (!Name.Contains("DrivingLane", ESearchCase::CaseSensitive))
            {
                Actor->SetActorHiddenInGame(true);
                HiddenActors.Add(Actor);
                continue;
            }
        }
        HiddenActors.Shrink();
    }

    auto Center = Bounds.GetCenter();
    auto Extent = Bounds.GetExtent();
    auto OrthoWidth = std::max(Extent.X, Extent.Y) * 2.0F;
    auto Location = FVector(Center.X, Center.Y, std::max(Bounds.Max.X, std::max(Bounds.Max.Y, Bounds.Max.Z)));
    auto Rotation = FRotator(-90, 0, 0);

    auto RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->AddToRoot();
    RenderTarget->ClearColor = FLinearColor::Black;
    RenderTarget->InitAutoFormat(1920, 1920);
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
    SCC2D->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
    SCC2D->bCaptureEveryFrame = false;
    SCC2D->bCaptureOnMovement = false;
    SCC2D->TextureTarget = RenderTarget;

    Camera->SetActorLocation(Location);
    Camera->SetActorRotation(Rotation);

    SCC2D->CaptureScene();
    TArray<FColor> Pixels;
    RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);

    auto Shape = FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY);
    TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
    auto PixelData = MakeUnique<TImagePixelData<FColor>>(Shape);
    PixelData->Pixels = Pixels;
    ImageTask->PixelData = MoveTemp(PixelData);

    FString ImagePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("carla-digitaltwins")) / TEXT("road_render.png");
    ImageTask->Filename = ImagePath;
    ImageTask->Format = EImageFormat::PNG;
    ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
    ImageTask->bOverwriteFile = true;
    ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));

    auto& HighResScreenshotConfig = GetHighResScreenshotConfig();
    auto Task = HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));

    for (auto& HiddenActor : HiddenActors)
        HiddenActor->SetActorHiddenInGame(false);

    Task.Wait();

    RunPythonRoadEdges();

    FString JsonPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("carla-digitaltwins")) / TEXT("contours.json");
    TArray<USplineComponent*> RoadSplines = UGeometryImporter::CreateSplinesFromJson(World, JsonPath);
    UE_LOG(LogTemp, Log, TEXT("Number of road splines: %i"), RoadSplines.Num());

    return nullptr;
}

UTexture2D* UOpenDriveToMap::RenderRoadToTexture(FString MapPath)
{
    auto World = UEditorLoadingAndSavingUtils::LoadMap(MapPath);
    return RenderRoadToTexture(World);
}

void UOpenDriveToMap::RunPythonRoadEdges()
{

  UE_LOG(LogTemp, Log, TEXT("Running Python road edges extraction script..."));
  
  FString PythonExe = PythonBinPath;
  FString PluginPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("carla-digitaltwins"));
  FString ScriptPath = PluginPath / TEXT("Content/Python/road_edge_detection.py");
  // Hardcoded values for testing

  FString Args;
  Args += FString::Printf(TEXT("\"%s\" "), *ScriptPath);
  Args += FString::Printf(TEXT("--plugin_path=\"%s\" "), *PluginPath);
  Args += FString::Printf(TEXT("--lon_min=%.8f "), OriginGeoCoordinates.Y);
  Args += FString::Printf(TEXT("--lat_min=%.8f "), OriginGeoCoordinates.X);
  Args += FString::Printf(TEXT("--lon_max=%.8f "), FinalGeoCoordinates.Y);
  Args += FString::Printf(TEXT("--lat_max=%.8f "), FinalGeoCoordinates.X);

  // Create communication pipes
  void* ReadPipe = nullptr;
  void* WritePipe = nullptr;
  FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

  // Launch process
  FProcHandle ProcHandle = FPlatformProcess::CreateProc(
      *PythonExe,
      *Args,
      true,   // bLaunchDetached
      false,  // bLaunchHidden
      false,  // bLaunchReallyHidden
      nullptr,
      0,
      nullptr,
      WritePipe,  // Pipe for stdout/stderr
      WritePipe
  );

  if (!ProcHandle.IsValid())
  {
      UE_LOG(LogTemp, Error, TEXT("Failed to launch Python script."));
      FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
      return;
  }

  // Read output
  FString Output;
  while (FPlatformProcess::IsProcRunning(ProcHandle))
  {
      FString NewOutput = FPlatformProcess::ReadPipe(ReadPipe);
      Output += NewOutput;
      FPlatformProcess::Sleep(0.01f);
  }

  // Read any remaining output
  Output += FPlatformProcess::ReadPipe(ReadPipe);

  // Clean up
  FPlatformProcess::CloseProc(ProcHandle);
  FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

  // Print result to Unreal log
  UE_LOG(LogTemp, Display, TEXT("Python Output:\n%s"), *Output);

}

#endif
