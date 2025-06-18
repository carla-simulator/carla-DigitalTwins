// Copyright (c) 2023 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.


#include "BlueprintUtil/BlueprintUtilFunctions.h"
#include "GeneralProjectSettings.h"
#include "Misc/Paths.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorAssetLibrary.h"

UObject* UBlueprintUtilFunctions::CopyAssetToPlugin(UObject* SourceObject, FString PluginName)
{
#if WITH_EDITOR
  if (!SourceObject || PluginName.IsEmpty()) {
    return nullptr;
  }

  FString SourcePath = SourceObject->GetPathName();
  FString SourcePackagePath = FPackageName::ObjectPathToPackageName(SourcePath);
  FString SourceAssetName = SourceObject->GetName();

  FString DestinationPath = FString::Printf(TEXT("/%s"), *PluginName);
  FString TargetAssetPath = TEXT("/" + PluginName) / SourceAssetName;

  if (UEditorAssetLibrary::DoesAssetExist(TargetAssetPath))
  {
    UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(TargetAssetPath);
    if (ExistingAsset)
    {
      return ExistingAsset;
    }
  }

  // Duplicate asset
  FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
  UObject* DuplicatedAsset = AssetToolsModule.Get().DuplicateAsset(SourceAssetName, DestinationPath, SourceObject);

  if (!DuplicatedAsset)
  {
    UE_LOG(LogTemp, Error, TEXT("Failed to duplicate asset."));
    return nullptr;
  }

  // Save the asset
  FString DuplicatedPath = DuplicatedAsset->GetPathName();
  if (!UEditorAssetLibrary::SaveAsset(DuplicatedPath, false))
  {
    UE_LOG(LogTemp, Warning, TEXT("Duplicated asset created but not saved: %s"), *DuplicatedPath);
  }

  return DuplicatedAsset;

#else
  return nullptr;
#endif
}

// /TestMap03/Static/DrivingLane/SM_DrivingLaneMesh9_X_0_Y_0.SM_DrivingLaneMesh9_X_0_Y_0
