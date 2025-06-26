// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "TrafficLights/TLHead.h"
#include "TrafficLights/TLHeadAttachment.h"
#include "TrafficLights/TLOrientation.h"
#include "TrafficLights/TLPole.h"
#include "TrafficLights/TLStyle.h"
#include "TrafficLights/Widgets/TLWTrafficLightPreviewViewport.h"
#include "Widgets/SCompoundWidget.h"

struct FEditorModule
{
	bool bExpanded{true};
	TSharedPtr<SVerticalBox> Container;
	TArray<TSharedPtr<FString>> MeshNameOptions;
};

struct FEditorHead
{
	bool bExpanded{true};
	bool bModulesExpanded{true};
	TSharedPtr<SVerticalBox> Container;
	TArray<FEditorModule> Modules;
};

struct FEditorPole
{
	bool bExpanded{true};
	bool bHeadsExpanded{true};
	TSharedPtr<SVerticalBox> Container;
	TArray<TSharedPtr<FString>> BaseMeshNameOptions;
	TArray<TSharedPtr<FString>> ExtendibleMeshNameOptions;
	TArray<TSharedPtr<FString>> FinalMeshNameOptions;
	TArray<FEditorHead> Heads;
};

class STrafficLightToolWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STrafficLightToolWidget)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void RefreshPoleList();
	void RebuildModuleChain(FTLHead& Head);
	void Rebuild();
	void RefreshModuleMeshOptions(int32 PoleIndex, int32 HeadIndex, int32 ModuleIndex);

private:
	FReply OnAddPoleClicked();
	FReply OnDeletePoleClicked(int32 PoleIndex);
	FReply OnAddHeadClicked(int32 PoleIndex);
	FReply OnDeleteHeadClicked(int32 PoleIndex, int32 HeadIndex);
	FReply OnAddModuleClicked(int32 PoleIndex, int32 HeadIndex);
	FReply OnDeleteModuleClicked(int32 PoleIndex, int32 HeadIndex, int32 ModuleIndex);
	FReply OnMoveModuleUpClicked(int32 PoleIndex, int32 HeadIndex, int32 ModuleIndex);
	FReply OnMoveModuleDownClicked(int32 PoleIndex, int32 HeadIndex, int32 ModuleIndex);
	void OnModuleVisorChanged(ECheckBoxState NewState, int32 PoleIndex, int32 HeadIndex, int32 ModuleIndex);
	void OnHeadOrientationChanged(ETLOrientation NewOrientation, int32 PoleIndex, int32 HeadIndex);
	void OnHeadStyleChanged(ETLStyle NewStyle, int32 PoleIndex, int32 HeadIndex);
	void OnPoleOrientationChanged(ETLOrientation NewOrientation, int32 PoleIndex);
	void OnPoleStyleChanged(ETLStyle NewStyle, int32 PoleIndex);

private:
	TSharedRef<SWidget> BuildPoleEntry(int32 Index);
	TSharedRef<SWidget> BuildPoleSection();
	TSharedRef<SWidget> BuildHeadEntry(int32 PoleIndex, int32 HeadIndex);
	TSharedRef<SWidget> BuildHeadSection(int32 HeadIndex);
	TSharedRef<SWidget> BuildModuleEntry(int32 PoleIndex, int32 HeadIndex, int32 ModuleIndex);
	TSharedRef<SWidget> BuildModuleSection(int32 PoleIndex, int32 HeadIndex);

private:
	FString GetHeadStyleText(ETLStyle Style);
	FString GetHeadAttachmentText(ETLHeadAttachment Attach);
	FString GetHeadOrientationText(ETLOrientation Orient);
	FString GetLightTypeText(ETLLightType Type);

private:
	TSharedPtr<STrafficLightPreviewViewport> PreviewViewport;
	TArray<FEditorPole> EditorPoles;
	TArray<FTLPole> Poles;
	TSharedPtr<SVerticalBox> RootContainer;

private:
	TArray<TSharedPtr<FString>> StyleOptions;
	TArray<TSharedPtr<FString>> OrientationOptions;
	TArray<TSharedPtr<FString>> AttachmentOptions;
	TArray<TSharedPtr<FString>> LightTypeOptions;
};
