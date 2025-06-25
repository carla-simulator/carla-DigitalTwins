// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "PreviewScene.h"
#include "EditorViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "TrafficLights/TLLightTypeDataTable.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLModule.h"
#include "TrafficLights/TLPole.h"

class STrafficLightPreviewViewport : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(STrafficLightPreviewViewport) {}
  SLATE_END_ARGS()

  /** Construct the preview viewport widget */
  void Construct(const FArguments &InArgs);

  /** Destructor: clear viewport reference to avoid crash */
  virtual ~STrafficLightPreviewViewport();

public:
  UPROPERTY(EditAnywhere, Category = "Light Type Data Table")
  UDataTable *LightTypesTable{nullptr};

  UPROPERTY(EditAnywhere, Category = "Modules Data Table")
  UDataTable *ModulesTable{nullptr};

  /** Head */
public:
  /** Set the head style  */
  void SetHeadStyle(int32 Index, ETLStyle Style);

  /** Modules */
public:
  UStaticMeshComponent *AddModuleMesh(const FTLHead &Head,
                                      FTLModule &ModuleData);
  void ClearModuleMeshes();
  void Rebuild(const TArray<FTLPole *> &Poles);
  void ResetFrame(const UStaticMeshComponent *Comp);

private:
  void LoadLightTypeDataTable();
  void LoadModulesDataTable();
  FVector2D GetAtlasCoordsForLightType(ETLLightType LightType) const;

private:
  TUniquePtr<FPreviewScene> PreviewScene;
  TSharedPtr<FEditorViewportClient> ViewportClient;
  TSharedPtr<FSceneViewport> SceneViewport;
  TSharedPtr<SViewport> ViewportWidget;

  TArray<UStaticMeshComponent *> HeadMeshComponents;
  TArray<UStaticMeshComponent *> ModuleMeshComponents;
};
