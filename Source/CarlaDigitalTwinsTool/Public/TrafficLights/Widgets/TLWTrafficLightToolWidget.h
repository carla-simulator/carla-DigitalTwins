// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLHeadAttachment.h"
#include "TrafficLights/TLOrientation.h"
#include "TrafficLights/TLStyle.h"
#include "TrafficLights/Widgets/TLWTrafficLightPreviewViewport.h"

struct FEditorModule {
  FTLModule Data;
  bool bExpanded{true};
  TSharedPtr<SVerticalBox> Container;
  TArray<TSharedPtr<FString>> MeshNameOptions;
};

struct FEditorHead {
  FTLHead Data;
  bool bExpanded{true};
  bool bModulesExpanded{true};
  TSharedPtr<SVerticalBox> Container;
  TArray<FEditorModule> Modules;
};

struct FEditorPole {
  FTLPole Data;
  bool bExpanded{true};
  bool bHeadsExpanded{true};
  TSharedPtr<SVerticalBox> Container;
  TArray<TSharedPtr<FString>> BaseMeshNameOptions;
  TArray<TSharedPtr<FString>> ExtendibleMeshNameOptions;
  TArray<TSharedPtr<FString>> FinalMeshNameOptions;
  TArray<FEditorHead> Heads;
};

class STrafficLightToolWidget : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(STrafficLightToolWidget) {}
  SLATE_END_ARGS()

  void Construct(const FArguments &InArgs);

private:
  void RefreshPoleList();
  void RebuildModuleChain(FEditorHead &EditorHead);
  void Rebuild();
  void RefreshModuleMeshOptions(int32 PoleIndex, int32 HeadIndex,
                                int32 ModuleIndex);
  TArray<FTLPole *> GetAllPoleDatas();

private:
  FReply OnAddPoleClicked();
  FReply OnDeletePoleClicked(int32 PoleIndex);
  FReply OnAddHeadClicked(int32 PoleIndex);
  FReply OnDeleteHeadClicked(int32 PoleIndex, int32 HeadIndex);
  FReply OnAddModuleClicked(int32 PoleIndex, int32 HeadIndex);
  FReply OnDeleteModuleClicked(int32 PoleIndex, int32 HeadIndex,
                               int32 ModuleIndex);
  FReply OnMoveModuleUpClicked(int32 PoleIndex, int32 HeadIndex,
                               int32 ModuleIndex);
  FReply OnMoveModuleDownClicked(int32 PoleIndex, int32 HeadIndex,
                                 int32 ModuleIndex);
  void OnModuleVisorChanged(ECheckBoxState NewState, int32 PoleIndex,
                            int32 HeadIndex, int32 ModuleIndex);
  void OnHeadOrientationChanged(ETLOrientation NewOrientation, int32 PoleIndex,
                                int32 HeadIndex);
  void OnHeadStyleChanged(ETLStyle NewStyle, int32 PoleIndex, int32 HeadIndex);

private:
  TSharedRef<SWidget> BuildPoleEntry(int32 Index);
  TSharedRef<SWidget> BuildPoleSection();
  TSharedRef<SWidget> BuildHeadEntry(int32 PoleIndex, int32 HeadIndex);
  TSharedRef<SWidget> BuildHeadSection(int32 HeadIndex);
  TSharedRef<SWidget> BuildModuleEntry(int32 PoleIndex, int32 HeadIndex,
                                       int32 ModuleIndex);
  TSharedRef<SWidget> BuildModuleSection(int32 PoleIndex, int32 HeadIndex);

private:
  FString GetHeadStyleText(ETLStyle Style);
  FString GetHeadAttachmentText(ETLHeadAttachment Attach);
  FString GetHeadOrientationText(ETLOrientation Orient);
  FString GetLightTypeText(ETLLightType Type);

private:
  TSharedPtr<STrafficLightPreviewViewport> PreviewViewport;
  TArray<FEditorPole> EditorPoles;
  TSharedPtr<SVerticalBox> PoleListContainer;

private:
  TArray<TSharedPtr<FString>> StyleOptions;
  TArray<TSharedPtr<FString>> OrientationOptions;
  TArray<TSharedPtr<FString>> AttachmentOptions;
  TArray<TSharedPtr<FString>> LightTypeOptions;
};
