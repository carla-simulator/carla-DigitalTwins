// Copyright (c) 2023 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.


#include "BlueprintUtil/BlueprintUtilFunctions.h"
#include "GeneralProjectSettings.h"
#include "Misc/Paths.h"
#include "AssetToolsModule.h"
#include "Engine/AssetManager.h"
#include "IAssetTools.h"
#include "ObjectTools.h"
#include "UObject/UObjectGlobals.h"
#include "EditorAssetLibrary.h"
#include "FileHelpers.h"

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

  TArray<UObject*> SubObjects;
  FReferenceFinder ReferenceCollector(SubObjects, nullptr, false, true, true);
  ReferenceCollector.FindReferences(DuplicatedAsset);

  TMap<UObject*, UObject*> ReplacementMap;
  TSet<UObject*> ObjectsToReplaceWithin;
  ObjectsToReplaceWithin.Add(DuplicatedAsset);

  for (UObject* RefObj : SubObjects)
  {
    if (RefObj)
    {
      FString RefObjPath = RefObj->GetPathName();
      FString TargetRefObjPath = TEXT("/") + PluginName / RefObj->GetName();

      UE_LOG(LogTemp, Log, TEXT("  Reference: %s (%s)"), *RefObj->GetName(), *RefObjPath);

      UObject* LoadedDuplicatedRefObject = nullptr;

      if (!UEditorAssetLibrary::DoesAssetExist(TargetRefObjPath) && RefObjPath.Contains(TEXT("/CarlaDigitalTwinsTool")))
      {
        // Duplicate the asset if it doesn't exist yet
        AssetToolsModule.Get().DuplicateAsset(*RefObj->GetName(), DestinationPath, RefObj);
      }

      // Try to load the duplicated or existing target asset
      LoadedDuplicatedRefObject = UEditorAssetLibrary::LoadAsset(TargetRefObjPath);

      if (LoadedDuplicatedRefObject)
      {
        UE_LOG(LogTemp, Log, TEXT("Replacement Map: %s -> %s"), *RefObjPath, *LoadedDuplicatedRefObject->GetPathName());
        ReplacementMap.Add(RefObj, LoadedDuplicatedRefObject);
        ObjectsToReplaceWithin.Add(LoadedDuplicatedRefObject);
      }
      else
      {
        UE_LOG(LogTemp, Error, TEXT("Failed to load duplicated or existing asset: %s"), *TargetRefObjPath);
      }
    }
  }

  // Rereference Assets
  for (const auto& Pair : ReplacementMap)
  {
    UObject* Original = Pair.Key;
    UObject* Replacement = Pair.Value;

    TArray<UObject*> ObjectsToReplace = {Original};
    ObjectTools::ForceReplaceReferences(Replacement, ObjectsToReplace, ObjectsToReplaceWithin);
  }

  //save all packages
  UE_LOG(LogTemp, Log, TEXT("Saving modified plugin assets..."));
  UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

  return DuplicatedAsset;

#else
  return nullptr;
#endif
}

