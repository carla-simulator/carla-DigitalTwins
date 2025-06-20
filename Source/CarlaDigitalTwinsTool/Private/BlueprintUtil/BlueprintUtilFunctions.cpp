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

DEFINE_LOG_CATEGORY(LogDigitalTwinsToolBlueprintUtil);

FString UBlueprintUtilFunctions::GetProjectName()
{
  const UGeneralProjectSettings* ProjectSettings = GetDefault<UGeneralProjectSettings>();
  return ProjectSettings->ProjectName;
}

UObject* UBlueprintUtilFunctions::CopyAssetToPlugin(UObject* SourceObject, FString PluginName)
{
#if WITH_EDITOR
  if (!SourceObject || PluginName.IsEmpty()) {
    return nullptr;
  }

  FString SourcePath = SourceObject->GetPathName();
  FString SourceAssetName = SourceObject->GetName();

  FString RootPath = TEXT("/CarlaDigitalTwinsTool/");
  int32 RootIndex = SourcePath.Find(RootPath);

  FString SourceFolderPath = FPaths::GetPath(SourcePath);

  FString RelativePath = SourceFolderPath.Mid(RootIndex + RootPath.Len());
  FString DestinationPath = FString::Printf(TEXT("/%s/%s"), *PluginName, *RelativePath);
  FString TargetAssetPath = DestinationPath + "/" + SourceAssetName;

  if (UEditorAssetLibrary::DoesAssetExist(TargetAssetPath))
  {
    UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(TargetAssetPath);
    if (ExistingAsset)
    {
      return ExistingAsset;
    }
  }

  FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
  UObject* DuplicatedAsset = AssetToolsModule.Get().DuplicateAsset(SourceAssetName, DestinationPath, SourceObject);

  if (!DuplicatedAsset)
  {
    UE_LOG(LogDigitalTwinsToolBlueprintUtil, Error, TEXT("Failed to duplicate asset."));
    return nullptr;
  }

  FString DuplicatedPath = DuplicatedAsset->GetPathName();
  if (!UEditorAssetLibrary::SaveAsset(DuplicatedPath, false))
  {
    UE_LOG(LogDigitalTwinsToolBlueprintUtil, Warning, TEXT("Duplicated asset created but not saved: %s"), *DuplicatedPath);
  }

  TSet<UObject*> SubObjects;
  TQueue<UObject*> Pending;
  Pending.Enqueue(DuplicatedAsset);

  while (!Pending.IsEmpty())
  {
    UObject* CurrentObject = nullptr;
    Pending.Dequeue(CurrentObject);

    if(!CurrentObject|| SubObjects.Contains(CurrentObject)) continue;

    SubObjects.Add(CurrentObject);

    TArray<UObject*> FoundRefs;
    FReferenceFinder ReferenceCollector(FoundRefs, nullptr, false, true, true);
    ReferenceCollector.FindReferences(CurrentObject);

    for (UObject* Ref : FoundRefs)
    {
      if (Ref && !SubObjects.Contains(Ref) && Ref->IsAsset())
      {
        Pending.Enqueue(Ref);
      }
    }
  }

  TMap<UObject*, UObject*> ReplacementMap;
  TSet<UObject*> ObjectsToReplaceWithin;
  ObjectsToReplaceWithin.Add(DuplicatedAsset);

  for (UObject* RefObj : SubObjects)
  {
    if (RefObj)
    {
      FString RefObjPath = RefObj->GetPathName();
      FString RootPath = TEXT("/CarlaDigitalTwinsTool/");
      int32 RootIndex = RefObjPath.Find(RootPath);

      FString SourceFolderPath = FPaths::GetPath(RefObjPath);
      FString RelativePath = SourceFolderPath.Mid(RootIndex + RootPath.Len());

      FString DestinationPath = FString::Printf(TEXT("/%s/%s"), *PluginName, *RelativePath);
      FString TargetRefObjPath = DestinationPath + "/" + RefObj->GetName();

      UObject* LoadedDuplicatedRefObject = nullptr;

      if (!UEditorAssetLibrary::DoesAssetExist(TargetRefObjPath) && RefObjPath.Contains(TEXT("/CarlaDigitalTwinsTool")))
      {
        AssetToolsModule.Get().DuplicateAsset(*RefObj->GetName(), DestinationPath, RefObj);
      }

      LoadedDuplicatedRefObject = UEditorAssetLibrary::LoadAsset(TargetRefObjPath);

      if (LoadedDuplicatedRefObject)
      {
        ReplacementMap.Add(RefObj, LoadedDuplicatedRefObject);
        ObjectsToReplaceWithin.Add(LoadedDuplicatedRefObject);
      }
    }
  }

  for (const auto& Pair : ReplacementMap)
  {
    UObject* Original = Pair.Key;
    UObject* Replacement = Pair.Value;

    TArray<UObject*> ObjectsToReplace = {Original};
    ObjectTools::ForceReplaceReferences(Replacement, ObjectsToReplace, ObjectsToReplaceWithin);
  }

  UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

  return DuplicatedAsset;

#else
  return nullptr;
#endif
}
