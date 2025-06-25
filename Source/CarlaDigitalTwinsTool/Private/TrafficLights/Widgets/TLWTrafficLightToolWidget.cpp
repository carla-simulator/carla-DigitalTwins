#include "Containers/UnrealString.h"
#include "Engine/StaticMeshSocket.h"
#include "Misc/AssertionMacros.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"

#include "TrafficLights/TLLightTypeDataTable.h"
#include "TrafficLights/TLMaterialFactory.h"
#include "TrafficLights/TLMeshFactory.h"
#include "TrafficLights/Widgets/TLWTrafficLightToolWidget.h"

void STrafficLightToolWidget::Construct(const FArguments &InArgs) {
  if (StyleOptions.Num() == 0) {
    if (UEnum *EnumPtr = StaticEnum<ETLStyle>()) {
      for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i) {
        StyleOptions.Add(MakeShared<FString>(
            EnumPtr->GetDisplayNameTextByIndex(i).ToString()));
      }
    }
  }
  if (AttachmentOptions.Num() == 0) {
    if (UEnum *EnumPtr = StaticEnum<ETLHeadAttachment>()) {
      for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i) {
        AttachmentOptions.Add(MakeShared<FString>(
            EnumPtr->GetDisplayNameTextByIndex(i).ToString()));
      }
    }
  }
  if (OrientationOptions.Num() == 0) {
    if (UEnum *EnumPtr = StaticEnum<ETLOrientation>()) {
      for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i) {
        OrientationOptions.Add(MakeShared<FString>(
            EnumPtr->GetDisplayNameTextByIndex(i).ToString()));
      }
    }
  }
  if (LightTypeOptions.Num() == 0) {
    if (UEnum *EnumPtr = StaticEnum<ETLLightType>()) {
      for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i) {
        LightTypeOptions.Add(MakeShared<FString>(
            EnumPtr->GetDisplayNameTextByIndex(i).ToString()));
      }
    }
  }

  ChildSlot[SNew(SSplitter)

            + SSplitter::Slot().Value(0.4f)[BuildPoleSection()]

            + SSplitter::Slot().Value(0.6f)[SAssignNew(
                  PreviewViewport, STrafficLightPreviewViewport)]];
  RefreshPoleList();
}

// ----------------------------------------------------------------------------------------------------------------
// ******************************   REBUILD
// ----------------------------------------------------------------------------------------------------------------
void STrafficLightToolWidget::Rebuild() {
  check(PreviewViewport.IsValid());

  for (int32 PoleIndex{0}; PoleIndex < EditorPoles.Num(); ++PoleIndex) {
    auto &EP{EditorPoles[PoleIndex]};
    for (int32 HeadIndex{0}; HeadIndex < EP.Heads.Num(); ++HeadIndex) {
      for (int32 ModuleIndex{0};
           ModuleIndex < EP.Heads[HeadIndex].Modules.Num(); ++ModuleIndex) {
        RefreshModuleMeshOptions(PoleIndex, HeadIndex, ModuleIndex);
      }
    }
  }

  for (auto &EP : EditorPoles) {
    for (auto &EH : EP.Heads) {
      RebuildModuleChain(EH);
    }
  }

  PreviewViewport->ClearModuleMeshes();
  PreviewViewport->Rebuild(GetAllPoleDatas());

  RefreshPoleList();
}

void STrafficLightToolWidget::RebuildModuleChain(FEditorHead &EditorHead) {
  TArray<FEditorModule> &EditorModules{EditorHead.Modules};
  if (EditorModules.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("RebuildModuleChain: Modules.Num()"));
    return;
  }

  static const FName EndSocketName{FName("Socket2")};
  static const FName BeginSocketName{FName("Socket1")};

  {
    FTLModule &Module{EditorModules[0].Data};
    Module.Transform = FTransform::Identity;
    if (Module.ModuleMeshComponent) {
      Module.ModuleMeshComponent->SetRelativeTransform(Module.Transform *
                                                       Module.Offset);
    }
  }

  for (int32 i{1}; i < EditorModules.Num(); ++i) {
    const FTLModule &Prev{EditorModules[i - 1].Data};
    FTLModule &Curr{EditorModules[i].Data};

    if (!Prev.ModuleMeshComponent || !Curr.ModuleMeshComponent) {
      UE_LOG(LogTemp, Warning, TEXT("Missing component at module %d"), i);
      continue;
    }
    const UStaticMeshSocket *PrevSocket{
        Prev.ModuleMesh->FindSocket(EndSocketName)};
    const UStaticMeshSocket *CurrSocket{
        Curr.ModuleMesh->FindSocket(BeginSocketName)};
    if (!PrevSocket || !CurrSocket) {
      UE_LOG(LogTemp, Warning, TEXT("Missing socket at module %d"), i);
      continue;
    }

    const FTransform PrevBase{Prev.Transform * Prev.Offset};
    const FTransform PrevLocal(FQuat::Identity, PrevSocket->RelativeLocation,
                               FVector::OneVector);
    const FTransform CurrLocal(FQuat::Identity, CurrSocket->RelativeLocation,
                               FVector::OneVector);
    const FTransform SnapDelta{PrevBase * PrevLocal * CurrLocal.Inverse()};
    Curr.Transform = SnapDelta;
    Curr.ModuleMeshComponent->SetRelativeTransform(Curr.Transform *
                                                   Curr.Offset);
  }
}

void STrafficLightToolWidget::RefreshPoleList() {
  check(PoleListContainer.IsValid());
  PoleListContainer->ClearChildren();

  for (int32 PoleIndex{0}; PoleIndex < EditorPoles.Num(); ++PoleIndex) {
    PoleListContainer->AddSlot().AutoHeight().Padding(
        2, 2)[BuildPoleEntry(PoleIndex)];
  }
}

void STrafficLightToolWidget::RefreshModuleMeshOptions(int32 PoleIndex,
                                                       int32 HeadIndex,
                                                       int32 ModuleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  auto &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  auto &Head{Pole.Heads[HeadIndex]};
  check(Head.Modules.IsValidIndex(ModuleIndex));
  auto &Module{Head.Modules[ModuleIndex]};

  TArray<UStaticMesh *> Meshes =
      FTLMeshFactory::GetAllMeshesForModule(Head.Data, Module.Data);
  Module.MeshNameOptions.Empty();
  for (UStaticMesh *Mesh : Meshes) {
    Module.MeshNameOptions.Add(MakeShared<FString>(Mesh->GetName()));
  }
}

TArray<FTLPole *> STrafficLightToolWidget::GetAllPoleDatas() {
  TArray<FTLPole *> PoleDatas;
  PoleDatas.Reserve(EditorPoles.Num());

  for (FEditorPole &Pole : EditorPoles) {
    PoleDatas.Add(&Pole.Data);
  }

  return PoleDatas;
}

// ----------------------------------------------------------------------------------------------------------------
// ******************************   EVENTS
// ----------------------------------------------------------------------------------------------------------------
FReply STrafficLightToolWidget::OnAddPoleClicked() {
  FEditorPole NewEditorPole;
  const int32 NewIndex{EditorPoles.Add(MoveTemp(NewEditorPole))};
  RefreshPoleList();
  { PreviewViewport->Rebuild(GetAllPoleDatas()); }

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeletePoleClicked(int32 PoleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  EditorPoles.RemoveAt(PoleIndex);
  RefreshPoleList();
  { PreviewViewport->Rebuild(GetAllPoleDatas()); }

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnAddHeadClicked(int32 PoleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &Pole{EditorPoles[PoleIndex]};

  const int32 HeadIndex{Pole.Heads.Emplace()};
  FEditorHead &NewHead{Pole.Heads[HeadIndex]};

  OnAddModuleClicked(PoleIndex, HeadIndex);
  Rebuild();
  if (NewHead.Modules.Num() > 0 &&
      NewHead.Modules[0].Data.ModuleMeshComponent) {
    PreviewViewport->ResetFrame(NewHead.Modules[0].Data.ModuleMeshComponent);
  }

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeleteHeadClicked(int32 PoleIndex,
                                                    int32 HeadIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &Pole{EditorPoles[PoleIndex]};

  check(Pole.Heads.IsValidIndex(HeadIndex));
  Pole.Heads.RemoveAt(HeadIndex);
  Rebuild();

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnAddModuleClicked(int32 PoleIndex,
                                                   int32 HeadIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &Head{Pole.Heads[HeadIndex]};

  FEditorModule NewEditorMod;
  NewEditorMod.Data.ModuleMesh =
      FTLMeshFactory::GetMeshForModule(Head.Data, NewEditorMod.Data);
  if (!NewEditorMod.Data.ModuleMesh) {
    UE_LOG(LogTemp, Error,
           TEXT("OnAddModuleClicked: failed to load the module."));
    return FReply::Handled();
  }

  UStaticMeshComponent *NewComp{
      PreviewViewport->AddModuleMesh(Head.Data, NewEditorMod.Data)};
  NewEditorMod.Data.ModuleMeshComponent = NewComp;
  Head.Data.Modules.Add(NewEditorMod.Data);
  Head.Modules.Add(MoveTemp(NewEditorMod));

  if (PoleIndex == 0 && HeadIndex == 0 && Head.Data.Modules.Num() == 1) {
    PreviewViewport->ResetFrame(NewComp);
  }
  Rebuild();

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeleteModuleClicked(int32 PoleIndex,
                                                      int32 HeadIndex,
                                                      int32 ModuleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &Head{Pole.Heads[HeadIndex]};

  check(Head.Modules.IsValidIndex(ModuleIndex));
  Head.Modules.RemoveAt(ModuleIndex);
  Head.Data.Modules.RemoveAt(ModuleIndex);
  Rebuild();

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnMoveModuleUpClicked(int32 PoleIndex,
                                                      int32 HeadIndex,
                                                      int32 ModuleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &Head{Pole.Heads[HeadIndex]};

  if (ModuleIndex > 0 && ModuleIndex < Head.Modules.Num()) {
    Head.Modules.Swap(ModuleIndex, ModuleIndex - 1);
    Rebuild();
  }

  return FReply::Handled();
}

FReply STrafficLightToolWidget::OnMoveModuleDownClicked(int32 PoleIndex,
                                                        int32 HeadIndex,
                                                        int32 ModuleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &Head{Pole.Heads[HeadIndex]};

  if (ModuleIndex >= 0 && ModuleIndex < Head.Modules.Num() - 1) {
    Head.Modules.Swap(ModuleIndex, ModuleIndex + 1);
    Rebuild();
  }

  return FReply::Handled();
}

void STrafficLightToolWidget::OnHeadOrientationChanged(
    ETLOrientation NewOrientation, int32 PoleIndex, int32 HeadIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &EditorPole{EditorPoles[PoleIndex]};
  check(EditorPole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &EditorHead{EditorPole.Heads[HeadIndex]};

  EditorHead.Data.Orientation = NewOrientation;
  for (FEditorModule &EditorModule : EditorHead.Modules) {
    UStaticMesh *NewMesh{
        FTLMeshFactory::GetMeshForModule(EditorHead.Data, EditorModule.Data)};
    if (!IsValid(NewMesh)) {
      UE_LOG(LogTemp, Error,
             TEXT("OnHeadOrientationChanged: failed to get mesh"));
      return;
    }
    EditorModule.Data.ModuleMesh = NewMesh;
    if (EditorModule.Data.ModuleMeshComponent) {
      EditorModule.Data.ModuleMeshComponent->SetStaticMesh(NewMesh);
    }
  }

  Rebuild();
}

void STrafficLightToolWidget::OnHeadStyleChanged(ETLStyle NewStyle,
                                                 int32 PoleIndex,
                                                 int32 HeadIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &EditorPole{EditorPoles[PoleIndex]};
  check(EditorPole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &EditorHead{EditorPole.Heads[HeadIndex]};

  EditorHead.Data.Style = NewStyle;
  for (FEditorModule &EditorModule : EditorHead.Modules) {
    UStaticMesh *NewMesh{
        FTLMeshFactory::GetMeshForModule(EditorHead.Data, EditorModule.Data)};
    if (!IsValid(NewMesh)) {
      UE_LOG(LogTemp, Error, TEXT("OnHeadStyleChanged: failed to get mesh"));
      return;
    }
    EditorModule.Data.ModuleMesh = NewMesh;
    if (EditorModule.Data.ModuleMeshComponent) {
      EditorModule.Data.ModuleMeshComponent->SetStaticMesh(NewMesh);
    }
  }

  Rebuild();
}

void STrafficLightToolWidget::OnModuleVisorChanged(ECheckBoxState NewState,
                                                   int32 PoleIndex,
                                                   int32 HeadIndex,
                                                   int32 ModuleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  FEditorPole &EditorPole{EditorPoles[PoleIndex]};
  check(EditorPole.Heads.IsValidIndex(HeadIndex));
  FEditorHead &EditorHead{EditorPole.Heads[HeadIndex]};
  check(EditorHead.Modules.IsValidIndex(ModuleIndex));
  FEditorModule &EditorModule{EditorHead.Modules[ModuleIndex]};

  EditorModule.Data.bHasVisor = (NewState == ECheckBoxState::Checked);
  UStaticMesh *NewMesh{
      FTLMeshFactory::GetMeshForModule(EditorHead.Data, EditorModule.Data)};
  if (!IsValid(NewMesh)) {
    UE_LOG(LogTemp, Error, TEXT("OnModuleVisorChanged: failed to get mesh"));
    return;
  }
  EditorModule.Data.ModuleMesh = NewMesh;
  if (EditorModule.Data.ModuleMeshComponent) {
    EditorModule.Data.ModuleMeshComponent->SetStaticMesh(NewMesh);
  }

  Rebuild();
}

// ----------------------------------------------------------------------------------------------------------------
// ******************************   BUILD SECTIONS
// ----------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> STrafficLightToolWidget::BuildPoleEntry(int32 PoleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  auto &EditorPole = EditorPoles[PoleIndex];
  FTLPole &PoleData = EditorPole.Data;

  auto BasePoleMeshes{FTLMeshFactory::GetAllBaseMeshesForPole(PoleData)};
  auto &BaseNames{EditorPole.BaseMeshNameOptions};
  BaseNames.Empty();
  for (UStaticMesh *M : BasePoleMeshes) {
    BaseNames.Add(MakeShared<FString>(M->GetName()));
  }

  auto ExtendiblePoleMeshes{
      FTLMeshFactory::GetAllExtendibleMeshesForPole(PoleData)};
  auto &ExtendibleNames{EditorPole.ExtendibleMeshNameOptions};
  ExtendibleNames.Empty();
  for (UStaticMesh *M : ExtendiblePoleMeshes) {
    ExtendibleNames.Add(MakeShared<FString>(M->GetName()));
  }

  auto FinalPoleMeshes{FTLMeshFactory::GetAllFinalMeshesForPole(PoleData)};
  auto &FinalNames{EditorPole.FinalMeshNameOptions};
  FinalNames.Empty();
  for (UStaticMesh *M : FinalPoleMeshes) {
    FinalNames.Add(MakeShared<FString>(M->GetName()));
  }

  TSharedPtr<FString> InitialBasePtr{nullptr};
  if (PoleData.BasePoleMesh) {
    const FString Curr{PoleData.BasePoleMesh->GetName()};
    for (auto &name : BaseNames)
      if (*name == Curr) {
        InitialBasePtr = name;
        break;
      }
  }

  TSharedPtr<FString> InitialExtendiblePtr{nullptr};
  if (PoleData.ExtendiblePoleMesh) {
    const FString Curr{PoleData.ExtendiblePoleMesh->GetName()};
    for (auto &name : ExtendibleNames)
      if (*name == Curr) {
        InitialExtendiblePtr = name;
        break;
      }
  }

  TSharedPtr<FString> InitialFinalPtr{nullptr};
  if (PoleData.FinalPoleMesh) {
    const FString Curr{PoleData.FinalPoleMesh->GetName()};
    for (auto &name : FinalNames)
      if (*name == Curr) {
        InitialFinalPtr = name;
        break;
      }
  }

  return SNew(SExpandableArea)
      .InitiallyCollapsed(!EditorPole.bExpanded)
      .AreaTitle(
          FText::FromString(FString::Printf(TEXT("Pole #%d"), PoleIndex)))
      .Padding(4)
      .OnAreaExpansionChanged_Lambda(
          [&EditorPole](bool bExpanded) { EditorPole.bExpanded = bExpanded; })
      // — Style —
      .BodyContent()
          [SNew(SVerticalBox)
           // — Base Mesh Combo —
           + SVerticalBox::Slot().AutoHeight().Padding(2, 4)
                 [SNew(SHorizontalBox) +
                  SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                      [SNew(STextBlock).Text(FText::FromString("Base Mesh:"))] +
                  SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                      [SNew(SComboBox<TSharedPtr<FString>>)
                           .OptionsSource(&BaseNames)
                           .InitiallySelectedItem(InitialBasePtr)
                           .OnGenerateWidget_Lambda(
                               [](TSharedPtr<FString> InPtr) {
                                 return SNew(STextBlock)
                                     .Text(FText::FromString(*InPtr));
                               })
                           .OnSelectionChanged_Lambda(
                               [this, PoleIndex](TSharedPtr<FString> New,
                                                 ESelectInfo::Type) {
                                 if (!New)
                                   return;
                                 check(EditorPoles.IsValidIndex(PoleIndex));
                                 FEditorPole &EP = EditorPoles[PoleIndex];
                                 FTLPole &PD = EP.Data;
                                 const FString SelectedName = *New;
                                 for (UStaticMesh *M :
                                      FTLMeshFactory::GetAllBaseMeshesForPole(
                                          PD)) {
                                   if (M && M->GetName() == SelectedName) {
                                     PD.BasePoleMesh = M;
                                     if (EP.Data.BasePoleMeshComponent) {
                                       EP.Data.BasePoleMeshComponent
                                           ->SetStaticMesh(M);
                                     }
                                     break;
                                   }
                                 }
                                 PreviewViewport->Rebuild(GetAllPoleDatas());
                               })[SNew(STextBlock).Text_Lambda([&]() {
                             return PoleData.BasePoleMesh
                                        ? FText::FromString(
                                              PoleData.BasePoleMesh->GetName())
                                        : FText::FromString("Select...");
                           })]]]
           // — Extendible Mesh Combo —
           + SVerticalBox::Slot().AutoHeight().Padding(
                 2,
                 4)[SNew(SHorizontalBox) +
                    SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [SNew(STextBlock)
                             .Text(FText::FromString("Extendible Mesh:"))] +
                    SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                        [SNew(SComboBox<TSharedPtr<FString>>)
                             .OptionsSource(&ExtendibleNames)
                             .InitiallySelectedItem(InitialExtendiblePtr)
                             .OnGenerateWidget_Lambda(
                                 [](TSharedPtr<FString> InPtr) {
                                   return SNew(STextBlock)
                                       .Text(FText::FromString(*InPtr));
                                 })
                             .OnSelectionChanged_Lambda(
                                 [this, PoleIndex](TSharedPtr<FString> NewSel,
                                                   ESelectInfo::Type) {
                                   if (!NewSel)
                                     return;
                                   check(EditorPoles.IsValidIndex(PoleIndex));
                                   FEditorPole &EP = EditorPoles[PoleIndex];
                                   FTLPole &PD = EP.Data;
                                   const FString SelectedName = *NewSel;
                                   for (UStaticMesh *M : FTLMeshFactory::
                                            GetAllExtendibleMeshesForPole(PD)) {
                                     if (M && M->GetName() == SelectedName) {
                                       PD.ExtendiblePoleMesh = M;
                                       if (PD.ExtendiblePoleMeshComponent) {
                                         PD.ExtendiblePoleMeshComponent
                                             ->SetStaticMesh(M);
                                       }
                                       break;
                                     }
                                   }
                                   PreviewViewport->Rebuild(GetAllPoleDatas());
                                 })[SNew(STextBlock).Text_Lambda([&]() {
                               return PoleData.ExtendiblePoleMesh
                                          ? FText::FromString(
                                                PoleData.ExtendiblePoleMesh
                                                    ->GetName())
                                          : FText::FromString("Select...");
                             })]]]
           // — Final Mesh Combo —
           + SVerticalBox::Slot().AutoHeight().Padding(2, 4)
                 [SNew(SHorizontalBox) +
                  SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                      [SNew(STextBlock).Text(FText::FromString("Top Mesh:"))] +
                  SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                      [SNew(SComboBox<TSharedPtr<FString>>)
                           .OptionsSource(&FinalNames)
                           .InitiallySelectedItem(InitialFinalPtr)
                           .OnGenerateWidget_Lambda(
                               [](TSharedPtr<FString> InPtr) {
                                 return SNew(STextBlock)
                                     .Text(FText::FromString(*InPtr));
                               })
                           .OnSelectionChanged_Lambda(
                               [this, PoleIndex](TSharedPtr<FString> NewSel,
                                                 ESelectInfo::Type) {
                                 if (!NewSel)
                                   return;
                                 check(EditorPoles.IsValidIndex(PoleIndex));
                                 FEditorPole &EP = EditorPoles[PoleIndex];
                                 FTLPole &PD = EP.Data;
                                 const FString SelectedName = *NewSel;
                                 for (UStaticMesh *M :
                                      FTLMeshFactory::GetAllFinalMeshesForPole(
                                          PD)) {
                                   if (M && M->GetName() == SelectedName) {
                                     PD.FinalPoleMesh = M;
                                     if (PD.FinalPoleMeshComponent) {
                                       PD.FinalPoleMeshComponent->SetStaticMesh(
                                           M);
                                     }
                                     break;
                                   }
                                 }
                                 PreviewViewport->Rebuild(GetAllPoleDatas());
                               })[SNew(STextBlock).Text_Lambda([&]() {
                             return PoleData.FinalPoleMesh
                                        ? FText::FromString(
                                              PoleData.FinalPoleMesh->GetName())
                                        : FText::FromString("Select...");
                           })]]]
           // — Pole Height Slider + Numeric Entry —
           + SVerticalBox::Slot().AutoHeight().Padding(2, 4)
                 [SNew(SHorizontalBox) +
                  SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                      [SNew(STextBlock)
                           .Text(FText::FromString("Pole Height (m):"))] +
                  SHorizontalBox::Slot().FillWidth(0.3f).Padding(8, 0)
                      [SNew(SNumericEntryBox<float>)
                           .Value_Lambda([&]() { return PoleData.PoleHeight; })
                           .OnValueChanged_Lambda([this,
                                                   PoleIndex](float NewVal) {
                             EditorPoles[PoleIndex].Data.PoleHeight = NewVal;
                             PreviewViewport->Rebuild(GetAllPoleDatas());
                           })] +
                  SHorizontalBox::Slot().FillWidth(0.7f).Padding(
                      8, 0)[SNew(SSlider)
                                .MinValue(0.0f)
                                .MaxValue(20.0f)
                                .Value_Lambda([&]() {
                                  return PoleData.PoleHeight / 20.0f;
                                })
                                .OnValueChanged_Lambda([this, PoleIndex](
                                                           float Normalized) {
                                  const float NewVal =
                                      FMath::Lerp(0.0f, 20.0f, Normalized);
                                  EditorPoles[PoleIndex].Data.PoleHeight =
                                      NewVal;
                                  PreviewViewport->Rebuild(GetAllPoleDatas());
                                })]]
           // — Heads Section —
           +
           SVerticalBox::Slot().AutoHeight().Padding(
               2, 8)[SNew(SExpandableArea)
                         .InitiallyCollapsed(!EditorPole.bHeadsExpanded)
                         .AreaTitle(FText::FromString("Heads"))
                         .Padding(4)
                         .OnAreaExpansionChanged_Lambda([&EditorPole](bool b) {
                           EditorPole.bHeadsExpanded = b;
                         })
                         .BodyContent()[
                             // Aquí metes tu BuildHeadSection que recorre todos
                             // los heads de este pole
                             BuildHeadSection(PoleIndex)]]
           // — Delete Pole button —
           + SVerticalBox::Slot()
                 .AutoHeight()
                 .HAlign(HAlign_Right)
                 .Padding(2, 0)[SNew(SButton)
                                    .Text(FText::FromString("Delete Pole"))
                                    .OnClicked_Lambda([this, PoleIndex]() {
                                      return OnDeletePoleClicked(PoleIndex);
                                    })]];
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildPoleSection() {
  SAssignNew(PoleListContainer, SVerticalBox);

  return SNew(SVerticalBox) +
         SVerticalBox::Slot().AutoHeight().Padding(
             10)[SNew(SButton)
                     .Text(FText::FromString("Add Pole"))
                     .OnClicked(this,
                                &STrafficLightToolWidget::OnAddPoleClicked)] +
         SVerticalBox::Slot().FillHeight(1.0f).Padding(
             10)[SNew(SScrollBox) +
                 SScrollBox::Slot()[PoleListContainer.ToSharedRef()]];
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildHeadEntry(int32 PoleIndex,
                                                            int32 HeadIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  auto &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  auto &Head{Pole.Heads[HeadIndex]};

  FTLHead &HeadData{Head.Data};

  static constexpr float PosMin{-50.0f};
  static constexpr float PosMax{50.0f};
  static constexpr float RotMin{0.0f};
  static constexpr float RotMax{360.0f};
  static constexpr float ScaleMin{0.1f};
  static constexpr float ScaleMax{10.0f};

  return SNew(SExpandableArea)
      .InitiallyCollapsed(!Head.bExpanded)
      .AreaTitle(
          FText::FromString(FString::Printf(TEXT("Head #%d"), HeadIndex)))
      .Padding(4)
      .OnAreaExpansionChanged_Lambda(
          [&Head](bool bExpanded) { Head.bExpanded = bExpanded; })
      .BodyContent()
          [SNew(SBorder)
               .BorderBackgroundColor(FLinearColor{0.15f, 0.15f, 0.15f})
               .Padding(8)
                   [SNew(SVerticalBox)

                    // — Head Style —
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(STextBlock)
                                 .Text(FText::FromString("Head Style"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SComboBox<TSharedPtr<FString>>)
                               .OptionsSource(&StyleOptions)
                               .InitiallySelectedItem(
                                   StyleOptions[(int32)HeadData.Style])
                               .OnGenerateWidget_Lambda(
                                   [](TSharedPtr<FString> In) {
                                     return SNew(STextBlock)
                                         .Text(FText::FromString(*In));
                                   })
                               .OnSelectionChanged_Lambda(
                                   [this, PoleIndex, HeadIndex,
                                    &Head](TSharedPtr<FString> New,
                                           ESelectInfo::Type) {
                                     int32 Choice{
                                         StyleOptions.IndexOfByPredicate(
                                             [&](auto &S) {
                                               return S == New;
                                             })};
                                     Head.Data.Style =
                                         static_cast<ETLStyle>(Choice);
                                     OnHeadStyleChanged(Head.Data.Style,
                                                        PoleIndex, HeadIndex);
                                   })[SNew(STextBlock)
                                          .Text_Lambda([this, PoleIndex,
                                                        HeadIndex, &Head]() {
                                            auto &HeadData{Head.Data};
                                            return FText::FromString(
                                                *StyleOptions[(int32)HeadData
                                                                  .Style]);
                                          })]]

                    // — Attachment Type —
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(STextBlock)
                                 .Text(FText::FromString("Attachment"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(2)
                        [SNew(SComboBox<TSharedPtr<FString>>)
                             .OptionsSource(&AttachmentOptions)
                             .InitiallySelectedItem(
                                 AttachmentOptions[(int32)HeadData.Attachment])
                             .OnGenerateWidget_Lambda(
                                 [](TSharedPtr<FString> In) {
                                   return SNew(STextBlock)
                                       .Text(FText::FromString(*In));
                                 })
                             .OnSelectionChanged_Lambda(
                                 [this, PoleIndex, HeadIndex,
                                  &Head](TSharedPtr<FString> New,
                                         ESelectInfo::Type) {
                                   int32 Choice{
                                       AttachmentOptions.IndexOfByPredicate(
                                           [&](auto &S) { return S == New; })};
                                   Head.Data.Attachment =
                                       static_cast<ETLHeadAttachment>(Choice);
                                 })[SNew(STextBlock)
                                        .Text_Lambda([this, PoleIndex,
                                                      HeadIndex, &Head]() {
                                          auto &HeadData{Head.Data};
                                          return FText::FromString(
                                              *AttachmentOptions
                                                  [(int32)HeadData.Attachment]);
                                        })]]

                    // — Orientation —
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(STextBlock)
                                 .Text(FText::FromString("Orientation"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SComboBox<TSharedPtr<FString>>)
                               .OptionsSource(&OrientationOptions)
                               .InitiallySelectedItem(
                                   OrientationOptions[(int32)
                                                          HeadData.Orientation])
                               .OnGenerateWidget_Lambda(
                                   [](TSharedPtr<FString> In) {
                                     return SNew(STextBlock)
                                         .Text(FText::FromString(*In));
                                   })
                               .OnSelectionChanged_Lambda(
                                   [this, PoleIndex,
                                    HeadIndex](TSharedPtr<FString> New,
                                               ESelectInfo::Type) {
                                     int32 Choice{
                                         OrientationOptions.IndexOfByPredicate(
                                             [&](auto &S) {
                                               return S == New;
                                             })};
                                     OnHeadOrientationChanged(
                                         static_cast<ETLOrientation>(Choice),
                                         PoleIndex, HeadIndex);
                                   })[SNew(STextBlock)
                                          .Text_Lambda([this, PoleIndex,
                                                        HeadIndex, &Head]() {
                                            auto &HeadData{Head.Data};
                                            return FText::FromString(
                                                *OrientationOptions
                                                    [(int32)
                                                         HeadData.Orientation]);
                                          })]]

                    // — Backplate —
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(SCheckBox)
                                 .IsChecked_Lambda(
                                     [this, PoleIndex, HeadIndex, &Head]() {
                                       return Head.Data.bHasBackplate
                                                  ? ECheckBoxState::Checked
                                                  : ECheckBoxState::Unchecked;
                                     })
                                 .OnCheckStateChanged_Lambda(
                                     [this, PoleIndex, HeadIndex,
                                      &Head](ECheckBoxState NewState) {
                                       bool bOn{(NewState ==
                                                 ECheckBoxState::Checked)};
                                       Head.Data.bHasBackplate = bOn;
                                       if (bOn) {
                                         // TODO: Add Backplate
                                       } else {
                                         // TODO: Remove Backplate
                                       }
                                     })[SNew(STextBlock)
                                            .Text(FText::FromString(
                                                "Has Backplate"))]]

                    // Relative Location
                    +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(STextBlock)
                               .Text(FText::FromString("Relative Location"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Transform
                                              .GetLocation()
                                              .X;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FVector L{
                                              HeadData.Transform.GetLocation()};
                                          L.X = V;
                                          HeadData.Transform.SetLocation(L);
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Transform
                                              .GetLocation()
                                              .Y;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FVector L{
                                              HeadData.Transform.GetLocation()};
                                          L.Y = V;
                                          HeadData.Transform.SetLocation(L);
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Transform
                                              .GetLocation()
                                              .Z;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FVector L{
                                              HeadData.Transform.GetLocation()};
                                          L.Z = V;
                                          HeadData.Transform.SetLocation(L);
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(PosMin)
                                        .MaxValue(PosMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Transform
                                                      .GetLocation()
                                                      .X -
                                                  PosMin) /
                                                 (PosMax - PosMin);
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float N) {
                                          const float V{
                                              FMath::Lerp(PosMin, PosMax, N)};
                                          FVector Loc{Head.Data.Transform
                                                          .GetLocation()};
                                          Loc.X = V;
                                          Head.Data.Transform.SetLocation(Loc);
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(PosMin)
                                        .MaxValue(PosMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Transform
                                                      .GetLocation()
                                                      .Y -
                                                  PosMin) /
                                                 (PosMax - PosMin);
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float N) {
                                          const float V{
                                              FMath::Lerp(PosMin, PosMax, N)};
                                          FVector Loc{Head.Data.Transform
                                                          .GetLocation()};
                                          Loc.Y = V;
                                          Head.Data.Transform.SetLocation(Loc);
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(PosMin)
                                        .MaxValue(PosMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Transform
                                                      .GetLocation()
                                                      .Z -
                                                  PosMin) /
                                                 (PosMax - PosMin);
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float N) {
                                          const float V{
                                              FMath::Lerp(PosMin, PosMax, N)};
                                          FVector Loc{Head.Data.Transform
                                                          .GetLocation()};
                                          Loc.Z = V;
                                          Head.Data.Transform.SetLocation(Loc);
                                        })]]

                    // Offset Position
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(STextBlock)
                                 .Text(FText::FromString("Offset Position"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.GetLocation()
                                              .X;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FVector L{
                                              HeadData.Offset.GetLocation()};
                                          L.X = V;
                                          HeadData.Offset.SetLocation(L);
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.GetLocation()
                                              .Y;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FVector L{
                                              HeadData.Offset.GetLocation()};
                                          L.Y = V;
                                          HeadData.Offset.SetLocation(L);
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.GetLocation()
                                              .Z;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FVector L{
                                              HeadData.Offset.GetLocation()};
                                          L.Z = V;
                                          HeadData.Offset.SetLocation(L);
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(PosMin)
                                        .MaxValue(PosMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Offset.GetLocation()
                                                      .X -
                                                  PosMin) /
                                                 (PosMax - PosMin);
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float N) {
                                          const float V{
                                              FMath::Lerp(PosMin, PosMax, N)};
                                          FVector Off{
                                              Head.Data.Offset.GetLocation()};
                                          Off.X = V;
                                          Head.Data.Offset.SetLocation(Off);
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(PosMin)
                                        .MaxValue(PosMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Offset.GetLocation()
                                                      .Y -
                                                  PosMin) /
                                                 (PosMax - PosMin);
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float N) {
                                          const float V{
                                              FMath::Lerp(PosMin, PosMax, N)};
                                          FVector Off{
                                              Head.Data.Offset.GetLocation()};
                                          Off.Y = V;
                                          Head.Data.Offset.SetLocation(Off);
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(PosMin)
                                        .MaxValue(PosMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Offset.GetLocation()
                                                      .Z -
                                                  PosMin) /
                                                 (PosMax - PosMin);
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float N) {
                                          const float V{
                                              FMath::Lerp(PosMin, PosMax, N)};
                                          FVector Off{
                                              Head.Data.Offset.GetLocation()};
                                          Off.Z = V;
                                          Head.Data.Offset.SetLocation(Off);
                                        })]]

                    // Offset Rotation
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(STextBlock)
                                 .Text(FText::FromString("Offset Rotation"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.Rotator()
                                              .Pitch;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FRotator R{HeadData.Offset.Rotator()};
                                          R.Pitch = V;
                                          HeadData.Offset.SetRotation(FQuat{R});
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.Rotator().Yaw;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FRotator R{HeadData.Offset.Rotator()};
                                          R.Yaw = V;
                                          HeadData.Offset.SetRotation(FQuat{R});
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.Rotator()
                                              .Roll;
                                        })
                                        .OnValueChanged_Lambda([this, &Head](
                                                                   float V) {
                                          auto &HeadData{Head.Data};
                                          FRotator R{HeadData.Offset.Rotator()};
                                          R.Roll = V;
                                          HeadData.Offset.SetRotation(FQuat{R});
                                          if (PreviewViewport.IsValid()) {
                                            Rebuild();
                                          }
                                        })]] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(RotMin)
                                        .MaxValue(RotMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Offset.Rotator()
                                                      .Pitch -
                                                  RotMin) /
                                                 (RotMax - RotMin);
                                        })
                                        .OnValueChanged_Lambda(
                                            [this, &Head](float N) {
                                              const float V{FMath::Lerp(
                                                  RotMin, RotMax, N)};
                                              FRotator R{
                                                  Head.Data.Offset.Rotator()};
                                              R.Pitch = V;
                                              Head.Data.Offset.SetRotation(
                                                  FQuat{R});
                                            })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(RotMin)
                                        .MaxValue(RotMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Offset.Rotator()
                                                      .Yaw -
                                                  RotMin) /
                                                 (RotMax - RotMin);
                                        })
                                        .OnValueChanged_Lambda(
                                            [this, &Head](float N) {
                                              const float V{FMath::Lerp(
                                                  RotMin, RotMax, N)};
                                              FRotator R{
                                                  Head.Data.Offset.Rotator()};
                                              R.Yaw = V;
                                              Head.Data.Offset.SetRotation(
                                                  FQuat{R});
                                            })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SSlider)
                                        .MinValue(RotMin)
                                        .MaxValue(RotMax)
                                        .Value_Lambda([this, &Head]() {
                                          return (Head.Data.Offset.Rotator()
                                                      .Roll -
                                                  RotMin) /
                                                 (RotMax - RotMin);
                                        })
                                        .OnValueChanged_Lambda(
                                            [this, &Head](float N) {
                                              const float V{FMath::Lerp(
                                                  RotMin, RotMax, N)};
                                              FRotator R{
                                                  Head.Data.Offset.Rotator()};
                                              R.Roll = V;
                                              Head.Data.Offset.SetRotation(
                                                  FQuat{R});
                                            })]]

                    // Offset Scale
                    + SVerticalBox::Slot().AutoHeight().Padding(
                          2)[SNew(STextBlock)
                                 .Text(FText::FromString("Offset Scale"))] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.GetScale3D()
                                              .X;
                                        })
                                        .OnValueChanged_Lambda(
                                            [this, &Head](float V) {
                                              auto &HeadData = Head.Data;
                                              FVector S{
                                                  HeadData.Offset.GetScale3D()};
                                              S.X = V;
                                              HeadData.Offset.SetScale3D(S);
                                              if (PreviewViewport.IsValid()) {
                                                Rebuild();
                                              }
                                            })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.GetScale3D()
                                              .Y;
                                        })
                                        .OnValueChanged_Lambda(
                                            [this, &Head](float V) {
                                              auto &HeadData = Head.Data;
                                              FVector S{
                                                  HeadData.Offset.GetScale3D()};
                                              S.Y = V;
                                              HeadData.Offset.SetScale3D(S);
                                              if (PreviewViewport.IsValid()) {
                                                Rebuild();
                                              }
                                            })]

                           + SHorizontalBox::Slot().FillWidth(
                                 1)[SNew(SNumericEntryBox<float>)
                                        .Value_Lambda([this, &Head]() {
                                          return Head.Data.Offset.GetScale3D()
                                              .Z;
                                        })
                                        .OnValueChanged_Lambda(
                                            [this, &Head](float V) {
                                              auto &HeadData = Head.Data;
                                              FVector S{
                                                  HeadData.Offset.GetScale3D()};
                                              S.Z = V;
                                              HeadData.Offset.SetScale3D(S);
                                              if (PreviewViewport.IsValid()) {
                                                Rebuild();
                                              }
                                            })]] +
                    SVerticalBox::Slot().AutoHeight().Padding(
                        2)[SNew(SSlider)
                               .MinValue(ScaleMin)
                               .MaxValue(ScaleMax)
                               .Value_Lambda([this, &Head]() {
                                 return (Head.Data.Offset.GetScale3D().X -
                                         ScaleMin) /
                                        (ScaleMax - ScaleMin);
                               })
                               .OnValueChanged_Lambda([this, &Head](float N) {
                                 const float V{
                                     FMath::Lerp(ScaleMin, ScaleMax, N)};
                                 FVector S{Head.Data.Offset.GetScale3D()};
                                 S.X = V;
                                 Head.Data.Offset.SetScale3D(S);
                               })]

                    // — Add + Modules + Delete —
                    +
                    SVerticalBox::Slot()
                        .AutoHeight()
                        .HAlign(HAlign_Right)
                        .Padding(2, 4, 2, 0)
                            [SNew(SVerticalBox)

                             // Add Module
                             + SVerticalBox::Slot()
                                   .AutoHeight()
                                   .HAlign(HAlign_Right)
                                   .Padding(
                                       2, 4)[SNew(SButton)
                                                 .Text(FText::FromString(
                                                     "Add Module"))
                                                 .OnClicked(
                                                     this,
                                                     &STrafficLightToolWidget::
                                                         OnAddModuleClicked,
                                                     PoleIndex, HeadIndex)]

                             // Expandable “Modules” section
                             + SVerticalBox::Slot().AutoHeight().Padding(2, 8)
                                   [SNew(SExpandableArea)
                                        .InitiallyCollapsed(
                                            !Head.bModulesExpanded)
                                        .AreaTitle(FText::FromString("Modules"))
                                        .Padding(4)
                                        .OnAreaExpansionChanged_Lambda(
                                            [&Head](bool bExp) {
                                              Head.bModulesExpanded = bExp;
                                            })
                                        .BodyContent()[BuildModuleSection(
                                            PoleIndex, HeadIndex)]]

                             // Delete Head
                             + SVerticalBox::Slot().AutoHeight().Padding(2, 0)
                                   [SNew(SButton)
                                        .Text(FText::FromString("Delete Head"))
                                        .OnClicked(this,
                                                   &STrafficLightToolWidget::
                                                       OnDeleteHeadClicked,
                                                   PoleIndex, HeadIndex)]]]];
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildHeadSection(int32 PoleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  auto &Pole{EditorPoles[PoleIndex]};

  TSharedRef<SVerticalBox> Container{SNew(SVerticalBox)};

  // — Add Head button —
  Container->AddSlot().AutoHeight().Padding(
      2)[SNew(SHorizontalBox)

         + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
               [SNew(SButton)
                    .Text(FText::FromString("Add Head"))
                    .OnClicked(this, &STrafficLightToolWidget::OnAddHeadClicked,
                               PoleIndex)]];

  // — Scrollable list of head entries —
  Container->AddSlot().FillHeight(1.0f).Padding(
      2)[SNew(SScrollBox) +
         SScrollBox::Slot()[SAssignNew(Pole.Container, SVerticalBox)]];

  Pole.Container->ClearChildren();
  for (int32 HeadIndex{0}; HeadIndex < Pole.Heads.Num(); ++HeadIndex) {
    auto &Head{Pole.Heads[HeadIndex]};
    const bool bInitiallyCollapsed{!Head.bExpanded};

    Pole.Container->AddSlot().AutoHeight().Padding(
        4, 2)[BuildHeadEntry(PoleIndex, HeadIndex)];
  }

  return Container;
}

TSharedRef<SWidget>
STrafficLightToolWidget::BuildModuleEntry(int32 PoleIndex, int32 HeadIndex,
                                          int32 ModuleIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  auto &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  auto &Head{Pole.Heads[HeadIndex]};
  check(Head.Modules.IsValidIndex(ModuleIndex));
  auto &Module{Head.Modules[ModuleIndex]};

  FTLModule &ModuleData{Module.Data};
  auto &MeshNames{Module.MeshNameOptions};
  TSharedPtr<FString> InitialMeshPtr{nullptr};
  if (ModuleData.ModuleMesh) {
    const FString CurrName{ModuleData.ModuleMesh->GetName()};
    for (auto &OptionPtr : MeshNames) {
      if (*OptionPtr == CurrName) {
        InitialMeshPtr = OptionPtr;
        break;
      }
    }
  }

  // TODO: Adjust values
  static constexpr float PosMin = -50.0f, PosMax = 50.0f;
  static constexpr float RotMin = 0.0f, RotMax = 360.0f;
  static constexpr float ScaleMin = 0.1f, ScaleMax = 10.0f;

  return SNew(SBorder).Padding(4)
      [SNew(SHorizontalBox) +
       SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
           [SNew(SVerticalBox) +
            SVerticalBox::Slot().AutoHeight()
                [SNew(SButton)
                     .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                     .ContentPadding(2)
                     .IsEnabled(ModuleIndex > 0)
                     .OnClicked(
                         this, &STrafficLightToolWidget::OnMoveModuleUpClicked,
                         PoleIndex, HeadIndex, ModuleIndex)[SNew(SImage).Image(
                         FAppStyle::Get().GetBrush("Icons.ArrowUp"))]] +
            SVerticalBox::Slot().AutoHeight()
                [SNew(SButton)
                     .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                     .ContentPadding(2)
                     .IsEnabled(ModuleIndex < Head.Modules.Num() - 1)
                     .OnClicked(
                         this,
                         &STrafficLightToolWidget::OnMoveModuleDownClicked,
                         PoleIndex, HeadIndex, ModuleIndex)[SNew(SImage).Image(
                         FAppStyle::Get().GetBrush("Icons.ArrowDown"))]]] +
       SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
           [SNew(SVerticalBox) +
            SVerticalBox::Slot().AutoHeight().Padding(
                0,
                2)[SNew(SHorizontalBox) +
                   SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                       [SNew(STextBlock).Text(FText::FromString("Mesh:"))] +
                   SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                       [SNew(SComboBox<TSharedPtr<FString>>)
                            .OptionsSource(&MeshNames)
                            .InitiallySelectedItem(InitialMeshPtr)
                            .OnGenerateWidget_Lambda(
                                [](TSharedPtr<FString> InPtr) {
                                  return SNew(STextBlock)
                                      .Text(FText::FromString(*InPtr));
                                })
                            .OnSelectionChanged_Lambda(
                                [this, PoleIndex, HeadIndex, ModuleIndex,
                                 &Head](TSharedPtr<FString> NewSel,
                                        ESelectInfo::Type) {
                                  if (!NewSel) {
                                    return;
                                  }
                                  auto &EData{Head.Modules[ModuleIndex].Data};
                                  TArray<UStaticMesh *> MeshOptions{
                                      FTLMeshFactory::GetAllMeshesForModule(
                                          Head.Data, EData)};
                                  for (int32 i{0}; i < MeshOptions.Num(); ++i) {
                                    if (MeshOptions[i] &&
                                        MeshOptions[i]->GetName() == *NewSel) {
                                      EData.ModuleMesh = MeshOptions[i];
                                      break;
                                    }
                                  }
                                  PreviewViewport->Rebuild(GetAllPoleDatas());
                                })[SNew(STextBlock)
                                       .Text_Lambda([this, PoleIndex, HeadIndex,
                                                     ModuleIndex, &Head]() {
                                         auto &EData{
                                             Head.Modules[ModuleIndex].Data};
                                         return FText::FromString(
                                             EData.ModuleMesh
                                                 ? EData.ModuleMesh->GetName()
                                                 : TEXT("Select..."));
                                       })]]]

            // — Light Type ComboBox —
            +
            SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                [SNew(SHorizontalBox)

                 // Label
                 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                       [SNew(STextBlock).Text(FText::FromString("Light Type:"))]

                 // Combo
                 +
                 SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                     [SNew(SComboBox<TSharedPtr<FString>>)
                          .OptionsSource(&LightTypeOptions)
                          .InitiallySelectedItem(LightTypeOptions[static_cast<
                              int32>(ModuleData.LightType)])
                          .OnGenerateWidget_Lambda(
                              [](TSharedPtr<FString> InItem) {
                                return SNew(STextBlock)
                                    .Text(FText::FromString(*InItem));
                              })
                          .OnSelectionChanged_Lambda([&](TSharedPtr<FString>
                                                             NewSel,
                                                         ESelectInfo::Type) {
                            const int32 Choice{
                                LightTypeOptions.IndexOfByPredicate(
                                    [&](auto &StrPtr) {
                                      return *StrPtr == *NewSel;
                                    })};
                            ModuleData.LightType =
                                static_cast<ETLLightType>(Choice);

                            if (PreviewViewport->LightTypesTable) {
                              static const UEnum *EnumPtr{
                                  StaticEnum<ETLLightType>()};
                              const FString EnumKey{
                                  EnumPtr->GetNameStringByValue(
                                      (int64)ModuleData.LightType)};
                              const FName RowName(*EnumKey);

                              if (const FTLLightTypeRow *
                                  Row{PreviewViewport->LightTypesTable
                                          ->FindRow<FTLLightTypeRow>(
                                              RowName,
                                              TEXT("Lookup LightType"))}) {
                                ModuleData.U = Row->AtlasCoords.X;
                                ModuleData.V = Row->AtlasCoords.Y;
                                ModuleData.EmissiveColor = Row->Color;

                                if (ModuleData.LightMID) {
                                  ModuleData.LightMID->SetVectorParameterValue(
                                      TEXT("Emissive Color"),
                                      ModuleData.EmissiveColor);
                                }
                              } else {
                                UE_LOG(
                                    LogTemp, Warning,
                                    TEXT("LightTypesTable: row not found '%s'"),
                                    *RowName.ToString());
                              }
                            }

                            UStaticMeshComponent *Comp{
                                ModuleData.ModuleMeshComponent};
                            if (Comp) {
                              if (UMaterialInstanceDynamic *
                                  LightMID{
                                      FMaterialFactory::
                                          GetLightMaterialInstance(Comp)}) {
                                LightMID->SetScalarParameterValue(
                                    TEXT("Emmisive Intensity"),
                                    ModuleData.EmissiveIntensity);
                                LightMID->SetVectorParameterValue(
                                    TEXT("Emissive Color"),
                                    ModuleData.EmissiveColor);
                                LightMID->SetScalarParameterValue(
                                    TEXT("Offset U"),
                                    static_cast<float>(ModuleData.U));
                                LightMID->SetScalarParameterValue(
                                    TEXT("Offset Y"),
                                    static_cast<float>(ModuleData.V));
                                Comp->SetMaterial(1, LightMID);
                                ModuleData.LightMID = LightMID;
                              }
                            }
                            Rebuild();
                          })[SNew(STextBlock).Text_Lambda([&]() {
                            return FText::FromString(
                                GetLightTypeText(ModuleData.LightType));
                          })]]]

            // — Emissive Intensity Slider —
            +
            SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                [SNew(SHorizontalBox)

                 // Label
                 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                       [SNew(STextBlock).Text(FText::FromString("Emissive:"))]

                 // Slider
                 + SHorizontalBox::Slot()
                       .FillWidth(1.0f)
                       .VAlign(VAlign_Center)
                       .Padding(
                           8, 0)[SNew(SSlider)
                                     .MinValue(0.0f)
                                     .MaxValue(1000.0f)
                                     .Value_Lambda([&]() {
                                       return ModuleData.EmissiveIntensity;
                                     })
                                     .OnValueChanged_Lambda([&](float NewVal) {
                                       ModuleData.EmissiveIntensity = NewVal;
                                       if (ModuleData.LightMID) {
                                         ModuleData.LightMID
                                             ->SetScalarParameterValue(
                                                 TEXT("Emmisive Intensity"),
                                                 ModuleData.EmissiveIntensity);
                                       }
                                       Rebuild();
                                     })]]

            // — Emissive Color Picker —
            +
            SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                [SNew(SBorder)
                     .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                     .Padding(4)[SNew(SColorPicker)
                                     .TargetColorAttribute_Lambda([&]() {
                                       return ModuleData.EmissiveColor;
                                     })
                                     .UseAlpha(false)
                                     .OnlyRefreshOnMouseUp(false)
                                     .OnColorCommitted_Lambda(
                                         [&](FLinearColor NewColor) {
                                           ModuleData.EmissiveColor = NewColor;
                                           if (ModuleData.LightMID) {
                                             ModuleData.LightMID
                                                 ->SetVectorParameterValue(
                                                     TEXT("Emissive Color"),
                                                     NewColor);
                                           } else {
                                             UE_LOG(
                                                 LogTemp, Warning,
                                                 TEXT(
                                                     "Module %d of Head %d has "
                                                     "no Material Instance."),
                                                 ModuleIndex, HeadIndex);
                                           }
                                           PreviewViewport->Rebuild(
                                               GetAllPoleDatas());
                                         })]]

            // — Visor checkbox —
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                  [SNew(SCheckBox)
                       .IsChecked_Lambda(
                           [this, PoleIndex, HeadIndex, ModuleIndex, &Head]() {
                             return Head.Modules[ModuleIndex].Data.bHasVisor
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                           })
                       .OnCheckStateChanged(
                           this, &STrafficLightToolWidget::OnModuleVisorChanged,
                           PoleIndex, HeadIndex, ModuleIndex)
                           [SNew(STextBlock)
                                .Text(FText::FromString("Has Visor"))]]

            // — Offset Location: Numeric + Slider per axis —
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                  [SNew(STextBlock).Text(FText::FromString("Offset Location"))]

            // X axis
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                  [SNew(SHorizontalBox)

                   // Label X
                   + SHorizontalBox::Slot()
                         .AutoWidth()
                         .VAlign(VAlign_Center)
                         .Padding(0, 0, 8, 0)[SNew(STextBlock)
                                                  .Text(FText::FromString("X"))]

                   // Numeric Entry
                   + SHorizontalBox::Slot().FillWidth(0.3f).Padding(
                         2, 0)[SNew(SNumericEntryBox<float>)
                                   .MinValue(PosMin)
                                   .MaxValue(PosMax)
                                   .Value_Lambda([&]() {
                                     return ModuleData.Offset.GetLocation().X;
                                   })
                                   .OnValueChanged_Lambda([&](float V) {
                                     FVector L{ModuleData.Offset.GetLocation()};
                                     L.X = V;
                                     ModuleData.Offset.SetLocation(L);
                                     if (PreviewViewport.IsValid()) {
                                       Rebuild();
                                     }
                                   })]

                   // Slider
                   + SHorizontalBox::Slot().FillWidth(0.7f).Padding(
                         2, 0)[SNew(SSlider)
                                   .MinValue(PosMin)
                                   .MaxValue(PosMax)
                                   .Value_Lambda([&]() {
                                     return ModuleData.Offset.GetLocation().X;
                                   })
                                   .OnValueChanged_Lambda([&](float V) {
                                     FVector L{ModuleData.Offset.GetLocation()};
                                     L.X = V;
                                     ModuleData.Offset.SetLocation(L);
                                     if (PreviewViewport.IsValid()) {
                                       Rebuild();
                                     }
                                   })]]

            // Y axis
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                  [SNew(SHorizontalBox) +
                   SHorizontalBox::Slot()
                       .AutoWidth()
                       .VAlign(VAlign_Center)
                       .Padding(0, 0, 8, 0)[SNew(STextBlock)
                                                .Text(FText::FromString("Y"))] +
                   SHorizontalBox::Slot().FillWidth(0.3f).Padding(
                       2, 0)[SNew(SNumericEntryBox<float>)
                                 .MinValue(PosMin)
                                 .MaxValue(PosMax)
                                 .Value_Lambda([&]() {
                                   return ModuleData.Offset.GetLocation().Y;
                                 })
                                 .OnValueChanged_Lambda([&](float V) {
                                   FVector L{ModuleData.Offset.GetLocation()};
                                   L.Y = V;
                                   ModuleData.Offset.SetLocation(L);
                                   if (PreviewViewport.IsValid()) {
                                     Rebuild();
                                   }
                                 })] +
                   SHorizontalBox::Slot().FillWidth(0.7f).Padding(
                       2, 0)[SNew(SSlider)
                                 .MinValue(PosMin)
                                 .MaxValue(PosMax)
                                 .Value_Lambda([&]() {
                                   return ModuleData.Offset.GetLocation().Y;
                                 })
                                 .OnValueChanged_Lambda([&](float V) {
                                   FVector L{ModuleData.Offset.GetLocation()};
                                   L.Y = V;
                                   ModuleData.Offset.SetLocation(L);
                                   if (PreviewViewport.IsValid()) {
                                     Rebuild();
                                   }
                                 })]]

            // Z axis
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                  [SNew(SHorizontalBox) +
                   SHorizontalBox::Slot()
                       .AutoWidth()
                       .VAlign(VAlign_Center)
                       .Padding(0, 0, 8, 0)[SNew(STextBlock)
                                                .Text(FText::FromString("Z"))] +
                   SHorizontalBox::Slot().FillWidth(0.3f).Padding(
                       2, 0)[SNew(SNumericEntryBox<float>)
                                 .MinValue(PosMin)
                                 .MaxValue(PosMax)
                                 .Value_Lambda([&]() {
                                   return ModuleData.Offset.GetLocation().Z;
                                 })
                                 .OnValueChanged_Lambda([&](float V) {
                                   FVector L{ModuleData.Offset.GetLocation()};
                                   L.Z = V;
                                   ModuleData.Offset.SetLocation(L);
                                   if (PreviewViewport.IsValid()) {
                                     Rebuild();
                                   }
                                 })] +
                   SHorizontalBox::Slot().FillWidth(0.7f).Padding(
                       2, 0)[SNew(SSlider)
                                 .MinValue(PosMin)
                                 .MaxValue(PosMax)
                                 .Value_Lambda([&]() {
                                   return ModuleData.Offset.GetLocation().Z;
                                 })
                                 .OnValueChanged_Lambda([&](float V) {
                                   FVector L{ModuleData.Offset.GetLocation()};
                                   L.Z = V;
                                   ModuleData.Offset.SetLocation(L);
                                   if (PreviewViewport.IsValid()) {
                                     Rebuild();
                                   }
                                 })]]

            //----------------------------------------------------------------
            // Offset Rotation (Pitch, Yaw, Roll)
            //----------------------------------------------------------------
            + SVerticalBox::Slot().AutoHeight().Padding(
                  0, 2)[SNew(STextBlock)
                            .Text(FText::FromString("Offset Rotation"))] +
            SVerticalBox::Slot().AutoHeight().Padding(
                0, 2)[SNew(SHorizontalBox)

                      // Pitch
                      + SHorizontalBox::Slot().FillWidth(1).Padding(
                            2, 0)[SNew(SNumericEntryBox<float>)
                                      .MinValue(RotMin)
                                      .MaxValue(RotMax)
                                      .Value_Lambda([&]() {
                                        return ModuleData.Offset.Rotator()
                                            .Pitch;
                                      })
                                      .OnValueChanged_Lambda([&](float V) {
                                        FRotator R{ModuleData.Offset.Rotator()};
                                        R.Pitch = V;
                                        ModuleData.Offset.SetRotation(FQuat(R));
                                        if (PreviewViewport.IsValid()) {
                                          Rebuild();
                                        }
                                      })]

                      // Yaw
                      + SHorizontalBox::Slot().FillWidth(1).Padding(
                            2, 0)[SNew(SNumericEntryBox<float>)
                                      .MinValue(RotMin)
                                      .MaxValue(RotMax)
                                      .Value_Lambda([&]() {
                                        return ModuleData.Offset.Rotator().Yaw;
                                      })
                                      .OnValueChanged_Lambda([&](float V) {
                                        FRotator R{ModuleData.Offset.Rotator()};
                                        R.Yaw = V;
                                        ModuleData.Offset.SetRotation(FQuat(R));
                                        if (PreviewViewport.IsValid()) {
                                          Rebuild();
                                        }
                                      })]

                      // Roll
                      + SHorizontalBox::Slot().FillWidth(1).Padding(
                            2, 0)[SNew(SNumericEntryBox<float>)
                                      .MinValue(RotMin)
                                      .MaxValue(RotMax)
                                      .Value_Lambda([&]() {
                                        return ModuleData.Offset.Rotator().Roll;
                                      })
                                      .OnValueChanged_Lambda([&](float V) {
                                        FRotator R{ModuleData.Offset.Rotator()};
                                        R.Roll = V;
                                        ModuleData.Offset.SetRotation(FQuat(R));
                                        if (PreviewViewport.IsValid()) {
                                          Rebuild();
                                        }
                                      })]]

            // — Delete Module button —
            + SVerticalBox::Slot().AutoHeight().Padding(
                  0, 2)[SNew(SButton)
                            .Text(FText::FromString("Delete Module"))
                            .OnClicked(
                                this,
                                &STrafficLightToolWidget::OnDeleteModuleClicked,
                                PoleIndex, HeadIndex, ModuleIndex)]]];
}

TSharedRef<SWidget>
STrafficLightToolWidget::BuildModuleSection(int32 PoleIndex, int32 HeadIndex) {
  check(EditorPoles.IsValidIndex(PoleIndex));
  auto &Pole{EditorPoles[PoleIndex]};
  check(Pole.Heads.IsValidIndex(HeadIndex));
  auto &Head{Pole.Heads[HeadIndex]};

  TSharedRef<SVerticalBox> Container{SNew(SVerticalBox)};

  for (int32 ModuleIndex{0}; ModuleIndex < Head.Modules.Num(); ++ModuleIndex) {
    auto &Module{Head.Modules[ModuleIndex]};

    Container->AddSlot().AutoHeight().Padding(
        4, 2)[SNew(SExpandableArea)
                  .InitiallyCollapsed(!Module.bExpanded)
                  .AreaTitle(FText::FromString(
                      FString::Printf(TEXT("Module %d"), ModuleIndex)))
                  .Padding(4)
                  .OnAreaExpansionChanged_Lambda([this, PoleIndex, HeadIndex,
                                                  ModuleIndex](bool bExpanded) {
                    EditorPoles[PoleIndex]
                        .Heads[HeadIndex]
                        .Modules[ModuleIndex]
                        .bExpanded = bExpanded;
                  })
                  .BodyContent()[BuildModuleEntry(PoleIndex, HeadIndex,
                                                  ModuleIndex)]];
  }

  return Container;
}

// ----------------------------------------------------------------------------------------------------------------
// ******************************   ENUM TO TEXT
// ----------------------------------------------------------------------------------------------------------------
FString STrafficLightToolWidget::GetHeadStyleText(ETLStyle Style) {
  const UEnum *EnumPtr{StaticEnum<ETLStyle>()};
  if (!IsValid(EnumPtr)) {
    return FString(TEXT("Unknown"));
  }
  return EnumPtr->GetDisplayNameTextByValue((int64)Style).ToString();
}

FString
STrafficLightToolWidget::GetHeadAttachmentText(ETLHeadAttachment Attach) {
  const UEnum *EnumPtr{StaticEnum<ETLHeadAttachment>()};
  if (!IsValid(EnumPtr)) {
    return FString(TEXT("Unknown"));
  }
  return EnumPtr->GetDisplayNameTextByValue((int64)Attach).ToString();
}

FString STrafficLightToolWidget::GetHeadOrientationText(ETLOrientation Orient) {
  const UEnum *EnumPtr{StaticEnum<ETLOrientation>()};
  if (!IsValid(EnumPtr)) {
    return FString(TEXT("Unknown"));
  }
  return EnumPtr->GetDisplayNameTextByValue((int64)Orient).ToString();
}

FString STrafficLightToolWidget::GetLightTypeText(ETLLightType Type) {
  const UEnum *EnumPtr{StaticEnum<ETLLightType>()};
  if (!IsValid(EnumPtr)) {
    return FString(TEXT("Unknown"));
  }
  return EnumPtr->GetDisplayNameTextByValue((int64)Type).ToString();
}
