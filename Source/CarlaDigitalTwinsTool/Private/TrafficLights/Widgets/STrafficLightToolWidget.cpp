#include "TrafficLights/Widgets/STrafficLightToolWidget.h"
#include "TrafficLights/MaterialFactory.h"
#include "TrafficLights/ModuleMeshFactory.h"
#include "Containers/UnrealString.h"
#include "Misc/AssertionMacros.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/STextComboBox.h"
#include "InputCoreTypes.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SComboButton.h"
#include "Styling/AppStyle.h"

void STrafficLightToolWidget::Construct(const FArguments& InArgs)
{
    if (HeadStyleOptions.Num() == 0)
    {
        if (UEnum* EnumPtr = StaticEnum<ETLHeadStyle>())
        {
            for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
            {
                FText Display = EnumPtr->GetDisplayNameTextByIndex(i);
                HeadStyleOptions.Add(MakeShared<FString>(Display.ToString()));
            }
        }
    }
    if (HeadAttachmentOptions.Num() == 0)
    {
        if (UEnum* EnumPtr = StaticEnum<ETLHeadAttachment>())
        {
            for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
            {
                FText Display = EnumPtr->GetDisplayNameTextByIndex(i);
                HeadAttachmentOptions.Add(MakeShared<FString>(Display.ToString()));
            }
        }
    }
    if (HeadOrientationOptions.Num() == 0)
    {
        if (UEnum* EnumPtr = StaticEnum<ETLHeadOrientation>())
        {
            for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
            {
                FText Display = EnumPtr->GetDisplayNameTextByIndex(i);
                HeadOrientationOptions.Add(MakeShared<FString>(Display.ToString()));
            }
        }
    }

    if (LightTypeOptions.Num() == 0)
    {
        if (UEnum* EnumPtr = StaticEnum<ETLLightType>())
        {
            for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
            {
                FText Display = EnumPtr->GetDisplayNameTextByIndex(i);
                LightTypeOptions.Add(MakeShared<FString>(Display.ToString()));
            }
        }
    }

    ChildSlot
    [
        SNew(SSplitter)
        + SSplitter::Slot().Value(0.4f)
        [
            SNew(SVerticalBox)

            // Add Head button
            + SVerticalBox::Slot().AutoHeight().Padding(10)
            [
                SNew(SButton)
                .Text(FText::FromString("Add Head"))
                .OnClicked(this, &STrafficLightToolWidget::OnAddHeadClicked)
            ]

            // Scrollable list of head entries
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(10)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SAssignNew(HeadListContainer, SVerticalBox)
                ]
            ]
        ]

        // Preview viewport on the right
        + SSplitter::Slot().Value(0.6f)
        [
            SAssignNew(PreviewViewport, STrafficLightPreviewViewport)
        ]
    ];
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildModuleEntry(int32 HeadIndex, int32 ModuleIndex)
{
    check(Heads.IsValidIndex(HeadIndex));
    check(Heads[HeadIndex].Modules.IsValidIndex(ModuleIndex));

    auto& Head    = Heads[HeadIndex];
    auto& Module  = Head.Modules[ModuleIndex];
    auto& MeshNames = ModuleMeshNameOptionsPerHead[HeadIndex][ModuleIndex];

    TSharedPtr<FString> InitialMeshPtr = nullptr;
    if (Module.ModuleMesh)
    {
        const FString CurrentName = Module.ModuleMesh->GetName();
        for (auto& OptionPtr : MeshNames)
        {
            if (*OptionPtr == CurrentName)
            {
                InitialMeshPtr = OptionPtr;
                break;
            }
        }
    }

    static constexpr float PosMin   = -50.0f, PosMax   =  50.0f;
    static constexpr float RotMin   =   0.0f, RotMax   = 360.0f;
    static constexpr float ScaleMin =   0.1f, ScaleMax =  10.0f;

    return SNew(SBorder)
        .Padding(4)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(2.0f)
                .IsEnabled(ModuleIndex > 0)
                .OnClicked(this, &STrafficLightToolWidget::OnMoveModuleUpClicked, HeadIndex, ModuleIndex)
                [
                    SNew(SImage)
                    .Image(FAppStyle::Get().GetBrush("Icons.ArrowUp"))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(2.0f)
                .IsEnabled(ModuleIndex < Heads[HeadIndex].Modules.Num() - 1)
                .OnClicked(this, &STrafficLightToolWidget::OnMoveModuleDownClicked, HeadIndex, ModuleIndex)
                [
                    SNew(SImage)
                    .Image(FAppStyle::Get().GetBrush("Icons.ArrowDown"))
                ]
            ]
        ]

        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)

            // — Module Meshes —
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0,2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Text(FText::FromString("Mesh:"))
                ]

                + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(8,0)
                [
                    SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&MeshNames)
                    .InitiallySelectedItem(InitialMeshPtr)
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> InPtr) {
                        return SNew(STextBlock).Text(FText::FromString(*InPtr));
                    })
                    .OnSelectionChanged_Lambda(
                        [this, HeadIndex, ModuleIndex](TSharedPtr<FString> NewSel, ESelectInfo::Type) {
                        if (!NewSel) return;
                        auto& Head   = Heads[HeadIndex];
                        auto& Module = Head.Modules[ModuleIndex];
                        TArray<UStaticMesh*> MeshOptions = FModuleMeshFactory::GetAllMeshesForModule(Head, Module);
                        int32 Idx = INDEX_NONE;
                        for (int32 i = 0; i < MeshOptions.Num(); ++i)
                        {
                            if (MeshOptions[i] && MeshOptions[i]->GetName() == *NewSel)
                            {
                            Idx = i;
                            break;
                            }
                        }

                        if (Idx != INDEX_NONE)
                        {
                            Module.ModuleMesh = MeshOptions[Idx];
                            PreviewViewport->Rebuild(Heads);
                        }
                        }
                    )
                    [
                    SNew(STextBlock)
                        .Text_Lambda([this, HeadIndex, ModuleIndex]() {
                        const UStaticMesh* M = Heads[HeadIndex].Modules[ModuleIndex].ModuleMesh;
                        return FText::FromString(M ? M->GetName() : TEXT("Select..."));
                        })
                    ]
                ]
            ]

            // — Light Type ComboBox —
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0,2)
            [
                SNew(SHorizontalBox)

                // Label
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Light Type:"))
                ]

                // Combo
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8,0)
                [
                    SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&LightTypeOptions)
                    .InitiallySelectedItem(
                        LightTypeOptions[
                            static_cast<int32>( Heads[HeadIndex].Modules[ModuleIndex].LightType )
                        ]
                    )
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
                        return SNew(STextBlock).Text(FText::FromString(*InItem));
                    })
                    .OnSelectionChanged_Lambda([&](TSharedPtr<FString> NewSel, ESelectInfo::Type) {
                        const int32 Choice = LightTypeOptions.IndexOfByPredicate(
                            [&](auto& StrPtr){ return *StrPtr == *NewSel; }
                        );

                        Module.LightType = static_cast<ETLLightType>(Choice);

                        if (PreviewViewport->LightTypesTable)
                        {
                            static const UEnum* EnumPtr {StaticEnum<ETLLightType>()};
                            const FString EnumKey {EnumPtr->GetNameStringByValue((int64)Module.LightType)};
                            const FName RowName(*EnumKey);

                            if (const FTLLightTypeRow* Row {PreviewViewport->LightTypesTable->FindRow<FTLLightTypeRow>(RowName, TEXT("Lookup LightType"))})
                            {
                                Module.U = Row->AtlasCoords.X;
                                Module.V = Row->AtlasCoords.Y;
                                Module.EmissiveColor   = Row->Color;

                                if (Module.LightMID)
                                {
                                    Module.LightMID->SetVectorParameterValue(
                                        TEXT("Emissive Color"),
                                        Module.EmissiveColor
                                    );
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning,
                                    TEXT("LightTypesTable: row not found '%s'"), *RowName.ToString());
                            }
                        }

                        UStaticMeshComponent* Comp = Module.ModuleMeshComponent;
                        if (Comp)
                        {
                            if (UMaterialInstanceDynamic* LightMID {FMaterialFactory::GetLightMaterialInstance(Comp)})
                            {
                                LightMID->SetScalarParameterValue(TEXT("Emmisive Intensity"), Module.EmissiveIntensity);
                                LightMID->SetVectorParameterValue(TEXT("Emissive Color"), Module.EmissiveColor);
                                LightMID->SetScalarParameterValue(TEXT("Offset U"), static_cast<float>(Module.U));
                                LightMID->SetScalarParameterValue(TEXT("Offset Y"), static_cast<float>(Module.V));
                                Comp->SetMaterial(1, LightMID);
                                Module.LightMID = LightMID;
                            }
                        }
                        Rebuild();
                    })
                    [
                        SNew(STextBlock)
                        .Text_Lambda([&]() {
                            return FText::FromString(
                                GetLightTypeText(
                                    Module.LightType
                                )
                            );
                        })
                    ]
                ]
            ]

            // — Emissive Intensity Slider —
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0,2)
            [
                SNew(SHorizontalBox)

                // Label
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Emissive:"))
                ]

                // Slider
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                .Padding(8,0)
                [
                    SNew(SSlider)
                    .MinValue(0.0f).MaxValue(1000.0f)
                    .Value_Lambda([&]() {
                        return Module.EmissiveIntensity;
                    })
                    .OnValueChanged_Lambda([&](float NewVal) {
                        Module.EmissiveIntensity = NewVal;
                        if (Module.LightMID)
                        {
                            Module.LightMID->SetScalarParameterValue(TEXT("Emmisive Intensity"), Module.EmissiveIntensity);
                        }
                        Rebuild();
                    })
                ]
            ]

            // — Emissive Color Picker —
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0,2)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(4)
                [
                    SNew(SColorPicker)
                    .TargetColorAttribute_Lambda([&]()
                    {
                        return Module.EmissiveColor;
                    })
                    .UseAlpha(false)
                    .OnlyRefreshOnMouseUp(false)
                    .OnColorCommitted_Lambda([&](FLinearColor NewColor)
                    {
                        Module.EmissiveColor = NewColor;
                        if (Module.LightMID)
                        {
                            Module.LightMID->SetVectorParameterValue(TEXT("Emissive Color"), NewColor);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Module %d of Head %d has no Material Instance."), ModuleIndex, HeadIndex);
                        }
                        PreviewViewport->Rebuild(Heads);
                    })
                ]
            ]

            // — Has Visor Checkbox —
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0,2)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([&]() {
                    return Module.bHasVisor
                        ? ECheckBoxState::Checked
                        : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged(this, &STrafficLightToolWidget::OnModuleVisorChanged, HeadIndex, ModuleIndex)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Has Visor"))
                ]
            ]

            // — Offset Location: Numeric + Slider per axis —
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(STextBlock).Text(FText::FromString("Offset Location"))
            ]

            // X axis
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(SHorizontalBox)

                // Label X
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                [
                    SNew(STextBlock).Text(FText::FromString("X"))
                ]

                // Numeric Entry
                + SHorizontalBox::Slot().FillWidth(0.3f).Padding(2,0)
                [
                    SNew(SNumericEntryBox<float>)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.GetLocation().X;
                    })
                    .OnValueChanged_Lambda([&](float V){
                        FVector L = Module.Offset.GetLocation();
                        L.X = V;
                        Module.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                // Slider
                + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2,0)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.GetLocation().X;
                    })
                    .OnValueChanged_Lambda([&](float V){
                        FVector L = Module.Offset.GetLocation();
                        L.X = V;
                        Module.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]

            // Y axis
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                [
                    SNew(STextBlock).Text(FText::FromString("Y"))
                ]
                + SHorizontalBox::Slot().FillWidth(0.3f).Padding(2,0)
                [
                    SNew(SNumericEntryBox<float>)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.GetLocation().Y;
                    })
                    .OnValueChanged_Lambda([&](float V){
                        FVector L = Module.Offset.GetLocation();
                        L.Y = V;
                        Module.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
                + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2,0)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.GetLocation().Y;
                    })
                    .OnValueChanged_Lambda([&](float V){
                        FVector L = Module.Offset.GetLocation();
                        L.Y = V;
                        Module.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                    {
                        Rebuild();
                    }
                    })
                ]
            ]

            // Z axis
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                [
                    SNew(STextBlock).Text(FText::FromString("Z"))
                ]
                + SHorizontalBox::Slot().FillWidth(0.3f).Padding(2,0)
                [
                    SNew(SNumericEntryBox<float>)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.GetLocation().Z;
                    })
                    .OnValueChanged_Lambda([&](float V){
                        FVector L = Module.Offset.GetLocation();
                        L.Z = V;
                        Module.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
                + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2,0)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.GetLocation().Z;
                    })
                    .OnValueChanged_Lambda([&](float V){
                        FVector L = Module.Offset.GetLocation();
                        L.Z = V;
                        Module.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]

            //----------------------------------------------------------------
            // Offset Rotation (Pitch, Yaw, Roll)
            //----------------------------------------------------------------
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(STextBlock).Text(FText::FromString("Offset Rotation"))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(SHorizontalBox)

                // Pitch
                + SHorizontalBox::Slot().FillWidth(1).Padding(2,0)
                [
                    SNew(SNumericEntryBox<float>)
                    .MinValue(RotMin).MaxValue(RotMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.Rotator().Pitch;
                    })
                    .OnValueChanged_Lambda([&](float V) {
                        FRotator R = Module.Offset.Rotator();
                        R.Pitch = V;
                        Module.Offset.SetRotation(FQuat(R));
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                // Yaw
                + SHorizontalBox::Slot().FillWidth(1).Padding(2,0)
                [
                    SNew(SNumericEntryBox<float>)
                    .MinValue(RotMin).MaxValue(RotMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.Rotator().Yaw;
                    })
                    .OnValueChanged_Lambda([&](float V) {
                        FRotator R = Module.Offset.Rotator();
                        R.Yaw = V;
                        Module.Offset.SetRotation(FQuat(R));
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                // Roll
                + SHorizontalBox::Slot().FillWidth(1).Padding(2,0)
                [
                    SNew(SNumericEntryBox<float>)
                    .MinValue(RotMin).MaxValue(RotMax)
                    .Value_Lambda([&]() {
                        return Module.Offset.Rotator().Roll;
                    })
                    .OnValueChanged_Lambda([&](float V) {
                        FRotator R = Module.Offset.Rotator();
                        R.Roll = V;
                        Module.Offset.SetRotation(FQuat(R));
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]

            // Delete Module button
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(SButton)
                .Text(FText::FromString("Delete Module"))
                .OnClicked(this, &STrafficLightToolWidget::OnDeleteModuleClicked, HeadIndex, ModuleIndex)
            ]
        ]
    ];
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildModulesSection(int32 HeadIndex)
{
    check(Heads.IsValidIndex(HeadIndex));
    TArray<bool>& ExpandedArr = ModuleExpandedStates[HeadIndex];

    TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

     Container->AddSlot()
    .AutoHeight();

    for (int32 ModuleIndex = 0; ModuleIndex < Heads[HeadIndex].Modules.Num(); ++ModuleIndex)
    {
        const bool bIsExpanded = ModuleExpandedStates[HeadIndex][ModuleIndex];
        Container->AddSlot()
            .AutoHeight()
            .Padding(4,2)
        [
            SNew(SExpandableArea)
            .InitiallyCollapsed(!bIsExpanded)
            .AreaTitle(FText::FromString(FString::Printf(TEXT("Module %d"), ModuleIndex)))
            .Padding(4)
            .OnAreaExpansionChanged_Lambda([this, HeadIndex, ModuleIndex](bool bExpanded){
                ModuleExpandedStates[HeadIndex][ModuleIndex] = bExpanded;
            })
            .BodyContent()
            [
                BuildModuleEntry(HeadIndex, ModuleIndex)
            ]
        ];
    }

    return Container;
}

FReply STrafficLightToolWidget::OnAddHeadClicked()
{
    FTLHead NewHead;
    const int32 Index {Heads.Add(NewHead)};
    HeadExpandedStates.Add(true);
    HeadModulesSectionExpandedStates.Add(true);
    ModuleExpandedStates.Add(TArray<bool>());
    OnAddModuleClicked(Index);
    Rebuild();
    if (Heads.Num() == 1)
    {
        PreviewViewport->ResetFrame(Heads[0].Modules[0].ModuleMeshComponent);
    }
    return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeleteHeadClicked(int32 Index)
{
    check(PreviewViewport.IsValid());
    check(Heads.IsValidIndex(Index));

    Heads[Index].Modules.Empty();
    Heads.RemoveAt(Index);
    HeadExpandedStates.RemoveAt(Index);
    HeadModulesSectionExpandedStates.RemoveAt(Index);
    ModuleExpandedStates.RemoveAt(Index);

    Rebuild();
    return FReply::Handled();
}

void STrafficLightToolWidget::OnModuleVisorChanged(ECheckBoxState NewState, int32 HeadIndex, int32 ModuleIndex)
{
    check(PreviewViewport.IsValid());
    check(Heads.IsValidIndex(HeadIndex));

    FTLHead& HeadData {Heads[HeadIndex]};
    check(HeadData.Modules.IsValidIndex(ModuleIndex));

    FTLModule& Module {HeadData.Modules[ModuleIndex]};
    Module.bHasVisor = (NewState == ECheckBoxState::Checked);

    UStaticMesh* NewMesh {FModuleMeshFactory::GetMeshForModule(HeadData, Module)};
    if (!NewMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("OnModuleVisorChanged: failed to get mesh for head %d, module %d"), HeadIndex, ModuleIndex);
        return;
    }

    Module.ModuleMesh = NewMesh;
    if (Module.ModuleMeshComponent)
    {
        Module.ModuleMeshComponent->SetStaticMesh(NewMesh);
    }

    Rebuild();
}

void STrafficLightToolWidget::OnHeadOrientationChanged(ETLHeadOrientation NewOrientation, int32 HeadIndex)
{
    check(PreviewViewport.IsValid());
    check(Heads.IsValidIndex(HeadIndex));

    FTLHead& HeadData {Heads[HeadIndex]};
    HeadData.Orientation = NewOrientation;

    for (FTLModule& Module : HeadData.Modules)
    {
        Module.ModuleMesh = FModuleMeshFactory::GetMeshForModule(HeadData, Module);

        if (Module.ModuleMeshComponent)
        {
            Module.ModuleMeshComponent->SetStaticMesh(Module.ModuleMesh);
        }
    }

    Rebuild();
}

void STrafficLightToolWidget::OnHeadStyleChanged(ETLHeadStyle NewStyle, int32 HeadIndex)
{
    check(PreviewViewport.IsValid());
    check(Heads.IsValidIndex(HeadIndex));

    FTLHead& HeadData {Heads[HeadIndex]};
    HeadData.Style = NewStyle;

    for (FTLModule& Module : HeadData.Modules)
    {
        Module.ModuleMesh = FModuleMeshFactory::GetMeshForModule(HeadData, Module);

        if (Module.ModuleMeshComponent)
        {
            Module.ModuleMeshComponent->SetStaticMesh(Module.ModuleMesh);
        }
    }

    Rebuild();
}

void STrafficLightToolWidget::RefreshHeadList()
{
    HeadListContainer->ClearChildren();

    for (int32 i = 0; i < Heads.Num(); ++i)
    {
        HeadListContainer->AddSlot().AutoHeight().Padding(5)
        [
            BuildHeadEntry(i)
        ];
    }
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildHeadEntry(int32 Index)
{
    static constexpr float PosMin   { -50.0f };
    static constexpr float PosMax   {  50.0f };
    static constexpr float RotMin   {   0.0f };
    static constexpr float RotMax   { 360.0f };
    static constexpr float ScaleMin {   0.1f };
    static constexpr float ScaleMax {  10.0f };

    const bool bIsExpanded {HeadExpandedStates[Index]};
    const FTLHead& Head {Heads[Index]};
    return SNew(SExpandableArea)
    .InitiallyCollapsed(!bIsExpanded)
    .AreaTitle(FText::FromString(FString::Printf(TEXT("Head #%d"), Index)))
    .Padding(4)
    .OnAreaExpansionChanged( FOnBooleanValueChanged::CreateLambda(
        [this, Index](bool bExpanded)
        {
            HeadExpandedStates[Index] = bExpanded;
        }))
    .BodyContent()
    [
        SNew(SBorder)
            .BorderBackgroundColor(FLinearColor{ 0.15f, 0.15f, 0.15f })
            .Padding(8)
        [
            SNew(SVerticalBox)

            // Head Style
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [ SNew(STextBlock).Text(FText::FromString("Head Style")) ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&HeadStyleOptions)
                .InitiallySelectedItem(HeadStyleOptions[(int32)Head.Style])
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
                {
                    return SNew(STextBlock).Text(FText::FromString(*InItem));
                })
                .OnSelectionChanged_Lambda([this,Index](TSharedPtr<FString> New, ESelectInfo::Type)
                {
                    const int32 Choice = HeadStyleOptions.IndexOfByPredicate([&](auto& S){ return S == New; });
                    Heads[Index].Style = static_cast<ETLHeadStyle>(Choice);
                    OnHeadStyleChanged(Heads[Index].Style, Index);
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this,Index](){ return FText::FromString(*HeadStyleOptions[(int32)Heads[Index].Style]); })
                ]
            ]

            // Attachment
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [ SNew(STextBlock).Text(FText::FromString("Attachment Type")) ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&HeadAttachmentOptions)
                .InitiallySelectedItem(HeadAttachmentOptions[(int32)Head.Attachment])
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
                {
                    return SNew(STextBlock).Text(FText::FromString(*InItem));
                })
                .OnSelectionChanged_Lambda([this,Index](TSharedPtr<FString> New, ESelectInfo::Type)
                {
                    const int32 Choice = HeadAttachmentOptions.IndexOfByPredicate([&](auto& S){ return S == New; });
                    Heads[Index].Attachment = static_cast<ETLHeadAttachment>(Choice);
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this,Index](){ return FText::FromString(*HeadAttachmentOptions[(int32)Heads[Index].Attachment]); })
                ]
            ]

            // Orientation
            + SVerticalBox::Slot().AutoHeight().Padding(0,2)
            [
                SNew(STextBlock).Text(FText::FromString("Orientation"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&HeadOrientationOptions)
                .InitiallySelectedItem(HeadOrientationOptions[(int32)Heads[Index].Orientation])
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
                    return SNew(STextBlock).Text(FText::FromString(*InItem));
                })
                .OnSelectionChanged_Lambda([this, Index](TSharedPtr<FString> NewValue, ESelectInfo::Type )
                {
                    const int32 Choice = HeadOrientationOptions.IndexOfByPredicate(
                        [&](auto& S){ return S == NewValue; }
                    );
                    ETLHeadOrientation NewOrient = static_cast<ETLHeadOrientation>(Choice);
                    OnHeadOrientationChanged(NewOrient, Index);
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, Index]() {
                        return FText::FromString(
                            *HeadOrientationOptions[(int32)Heads[Index].Orientation]
                        );
                    })
                ]
            ]

            // Backplate checkbox + spawn/despawn
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this, Index]() {
                    return Heads[Index].bHasBackplate
                        ? ECheckBoxState::Checked
                        : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this, Index](ECheckBoxState State) {
                    const bool bNowHas = (State == ECheckBoxState::Checked);
                    Heads[Index].bHasBackplate = bNowHas;

                    if (bNowHas)
                    {
                        //PreviewViewport->AddBackplateMesh(Index);
                    }
                    else
                    {
                        //PreviewViewport->RemoveBackplateMesh(Index);
                    }
                })
                [
                    SNew(STextBlock).Text(FText::FromString("Has Backplate"))
                ]
            ]

            // Relative Location
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(STextBlock).Text(FText::FromString("Relative Location"))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Transform.GetLocation().X;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector L = Head.Transform.GetLocation();
                        L.X = V;
                        Head.Transform.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Transform.GetLocation().Y;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector L = Head.Transform.GetLocation();
                        L.Y = V;
                        Head.Transform.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Transform.GetLocation().Z;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector L = Head.Transform.GetLocation();
                        L.Z = V;
                        Head.Transform.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Transform.GetLocation().X - PosMin) / (PosMax - PosMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(PosMin, PosMax, N)};
                        FVector Loc{ Heads[Index].Transform.GetLocation() };
                        Loc.X = V;
                        Heads[Index].Transform.SetLocation(Loc);
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Transform.GetLocation().Y - PosMin) / (PosMax - PosMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(PosMin, PosMax, N)};
                        FVector Loc{ Heads[Index].Transform.GetLocation() };
                        Loc.Y = V;
                        Heads[Index].Transform.SetLocation(Loc);
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Transform.GetLocation().Z - PosMin) / (PosMax - PosMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(PosMin, PosMax, N)};
                        FVector Loc{ Heads[Index].Transform.GetLocation() };
                        Loc.Z = V;
                        Heads[Index].Transform.SetLocation(Loc);
                    })
                ]
            ]

            // Offset Position
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(STextBlock).Text(FText::FromString("Offset Position"))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Offset.GetLocation().X;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector L = Head.Offset.GetLocation();
                        L.X = V;
                        Head.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Offset.GetLocation().Y;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector L = Head.Offset.GetLocation();
                        L.Y = V;
                        Head.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Offset.GetLocation().Z;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector L = Head.Offset.GetLocation();
                        L.Z = V;
                        Head.Offset.SetLocation(L);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Offset.GetLocation().X - PosMin) / (PosMax - PosMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(PosMin, PosMax, N)};
                        FVector Off{ Heads[Index].Offset.GetLocation() };
                        Off.X = V;
                        Heads[Index].Offset.SetLocation(Off);
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Offset.GetLocation().Y - PosMin) / (PosMax - PosMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(PosMin, PosMax, N)};
                        FVector Off{ Heads[Index].Offset.GetLocation() };
                        Off.Y = V;
                        Heads[Index].Offset.SetLocation(Off);
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(PosMin).MaxValue(PosMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Offset.GetLocation().Z - PosMin) / (PosMax - PosMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(PosMin, PosMax, N)};
                        FVector Off{ Heads[Index].Offset.GetLocation() };
                        Off.Z = V;
                        Heads[Index].Offset.SetLocation(Off);
                    })
                ]
            ]

            // Offset Rotation
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(STextBlock).Text(FText::FromString("Offset Rotation"))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Offset.Rotator().Pitch;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FRotator R{ Head.Offset.Rotator() };
                        R.Pitch = V;
                        Head.Offset.SetRotation(FQuat{ R });
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Offset.Rotator().Yaw;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FRotator R{ Head.Offset.Rotator() };
                        R.Yaw = V;
                        Head.Offset.SetRotation(FQuat{ R });
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() {
                        return Heads[Index].Offset.Rotator().Roll;
                    })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FRotator R{ Head.Offset.Rotator() };
                        R.Roll = V;
                        Head.Offset.SetRotation(FQuat{ R });
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(RotMin).MaxValue(RotMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Offset.Rotator().Pitch - RotMin) / (RotMax - RotMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(RotMin, RotMax, N)};
                        FRotator R{ Heads[Index].Offset.Rotator() };
                        R.Pitch = V;
                        Heads[Index].Offset.SetRotation(FQuat{ R });
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(RotMin).MaxValue(RotMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Offset.Rotator().Yaw - RotMin) / (RotMax - RotMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(RotMin, RotMax, N)};
                        FRotator R{ Heads[Index].Offset.Rotator() };
                        R.Yaw = V;
                        Heads[Index].Offset.SetRotation(FQuat{ R });
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SSlider)
                    .MinValue(RotMin).MaxValue(RotMax)
                    .Value_Lambda([this,Index]() {
                        return (Heads[Index].Offset.Rotator().Roll - RotMin) / (RotMax - RotMin);
                    })
                    .OnValueChanged_Lambda([this,Index](float N){
                        const float V {FMath::Lerp(RotMin, RotMax, N)};
                        FRotator R{ Heads[Index].Offset.Rotator() };
                        R.Roll = V;
                        Heads[Index].Offset.SetRotation(FQuat{ R });
                    })
                ]
            ]

            // Offset Scale
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(STextBlock).Text(FText::FromString("Offset Scale"))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() { return Heads[Index].Offset.GetScale3D().X; })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector S{ Head.Offset.GetScale3D() };
                        S.X = V;
                        Head.Offset.SetScale3D(S);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() { return Heads[Index].Offset.GetScale3D().Y; })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector S{ Head.Offset.GetScale3D() };
                        S.Y = V;
                        Head.Offset.SetScale3D(S);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]

                + SHorizontalBox::Slot().FillWidth(1)
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([this,Index]() { return Heads[Index].Offset.GetScale3D().Z; })
                    .OnValueChanged_Lambda([this,Index](float V){
                        auto& Head = Heads[Index];
                        FVector S{ Head.Offset.GetScale3D() };
                        S.Z = V;
                        Head.Offset.SetScale3D(S);
                        if (PreviewViewport.IsValid())
                        {
                            Rebuild();
                        }
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(2)
            [
                SNew(SSlider)
                .MinValue(ScaleMin).MaxValue(ScaleMax)
                .Value_Lambda([this,Index]() {
                    return (Heads[Index].Offset.GetScale3D().X - ScaleMin) / (ScaleMax - ScaleMin);
                })
                .OnValueChanged_Lambda([this,Index](float N){
                    const float V {FMath::Lerp(ScaleMin, ScaleMax, N)};
                    FVector S{ Heads[Index].Offset.GetScale3D() };
                    S.X = V;
                    Heads[Index].Offset.SetScale3D(S);
                })
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Right)
            .Padding(2,4,2,0)
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Right)
                .Padding(2,4)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Add Module"))
                    .OnClicked_Lambda([this, Index]() {
                        OnAddModuleClicked(Index);
                        RefreshHeadList();
                        return FReply::Handled();
                    })
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2,8)
                [
                    SNew(SExpandableArea)
                    .InitiallyCollapsed(!HeadModulesSectionExpandedStates[Index])
                    .AreaTitle(FText::FromString("Modules"))
                    .Padding(4)
                    .OnAreaExpansionChanged_Lambda([this, Index](bool bExpanded){
                        HeadModulesSectionExpandedStates[Index] = bExpanded;
                    })
                    .BodyContent()
                    [
                    BuildModulesSection(Index)
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2,0)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Delete"))
                    .OnClicked_Lambda([this,Index]() { return OnDeleteHeadClicked(Index); })
                ]
            ]
        ]
    ];
}

void STrafficLightToolWidget::RebuildModuleChain(FTLHead& Head)
{
    if (Head.Modules.Num() == 0)
    {
        return;
    }

    const FName EndSocketName   {FName("Socket2")};
    const FName BeginSocketName {FName("Socket1")};

    {
        FTLModule& M0 {Head.Modules[0]};
        M0.Transform = FTransform::Identity;
        if (M0.ModuleMeshComponent)
        {
            M0.ModuleMeshComponent->SetRelativeTransform(M0.Transform * M0.Offset);
        }
    }

    for (int32 i {1}; i < Head.Modules.Num(); ++i)
    {
        const FTLModule& Prev {Head.Modules[i - 1]};
        FTLModule& Curr {Head.Modules[i]};

        if (!Prev.ModuleMeshComponent || !Curr.ModuleMeshComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("Missing component at module %d"), i);
            continue;
        }

        const UStaticMeshSocket* PrevSocket {Prev.ModuleMesh->FindSocket(EndSocketName)};
        const UStaticMeshSocket* CurrSocket {Curr.ModuleMesh->FindSocket(BeginSocketName)};

        if (!PrevSocket || !CurrSocket)
        {
            UE_LOG(LogTemp, Warning, TEXT("Missing socket at module %d"), i);
            continue;
        }

        const FTransform PrevBase {Prev.Transform * Prev.Offset};

        const FTransform PrevLocal(
            FQuat::Identity,
            PrevSocket->RelativeLocation,
            FVector::OneVector
        );
        const FTransform CurrLocal(
            FQuat::Identity,
            CurrSocket->RelativeLocation,
            FVector::OneVector
        );
        const FTransform SnapDelta {PrevBase * PrevLocal * CurrLocal.Inverse()};
        Curr.Transform = SnapDelta;
        Curr.ModuleMeshComponent->SetRelativeTransform(Curr.Transform * Curr.Offset);
    }
}

FReply STrafficLightToolWidget::OnAddModuleClicked(int32 HeadIndex)
{
    check(Heads.IsValidIndex(HeadIndex));

    ModuleExpandedStates[HeadIndex].Add(true);
    FTLHead& HeadData = Heads[HeadIndex];
    FTLModule NewModule;
    NewModule.ModuleMesh = FModuleMeshFactory::GetMeshForModule(HeadData, NewModule);

    if (!NewModule.ModuleMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load module mesh."));
        return FReply::Handled();
    }

    HeadData.Modules.Add(NewModule);
    FTLModule& StoredModule {HeadData.Modules.Last()};
    StoredModule.ModuleMeshComponent = PreviewViewport->AddModuleMesh(HeadData, StoredModule);
    Rebuild();

    return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeleteModuleClicked(int32 HeadIndex, int32 ModuleIndex)
{
    check(Heads.IsValidIndex(HeadIndex));
    auto& Head = Heads[HeadIndex];
    check(Head.Modules.IsValidIndex(ModuleIndex));
    Head.Modules.RemoveAt(ModuleIndex);
    ModuleExpandedStates[HeadIndex].RemoveAt(ModuleIndex);
    Rebuild();

    return FReply::Handled();
}

void STrafficLightToolWidget::Rebuild()
{
    check(PreviewViewport.IsValid());
    RefreshModuleMeshOptions();
    PreviewViewport->ClearModuleMeshes();
    for (FTLHead& Head : Heads)
    {
        RebuildModuleChain(Head);
    }
    PreviewViewport->Rebuild(Heads);
    RefreshHeadList();
}

FString STrafficLightToolWidget::GetHeadStyleText(ETLHeadStyle Style)
{
    const UEnum* EnumPtr = StaticEnum<ETLHeadStyle>();
    if (!EnumPtr) return FString(TEXT("Unknown"));
    return EnumPtr->GetDisplayNameTextByValue((int64)Style).ToString();
}

FString STrafficLightToolWidget::GetHeadAttachmentText(ETLHeadAttachment Attach)
{
    const UEnum* EnumPtr = StaticEnum<ETLHeadAttachment>();
    if (!EnumPtr) return FString(TEXT("Unknown"));
    return EnumPtr->GetDisplayNameTextByValue((int64)Attach).ToString();
}

FString STrafficLightToolWidget::GetHeadOrientationText(ETLHeadOrientation Orient)
{
    const UEnum* EnumPtr = StaticEnum<ETLHeadOrientation>();
    if (!EnumPtr) return FString(TEXT("Unknown"));
    return EnumPtr->GetDisplayNameTextByValue((int64)Orient).ToString();
}

FString STrafficLightToolWidget::GetLightTypeText(ETLLightType Type)
{
    const UEnum* EnumPtr = StaticEnum<ETLLightType>();
    if (!EnumPtr) return FString(TEXT("Unknown"));
    return EnumPtr->GetDisplayNameTextByValue((int64)Type).ToString();
}

void STrafficLightToolWidget::ChangeModulesOrientation(int32 HeadIndex, ETLHeadOrientation NewOrientation)
{
    check(PreviewViewport.IsValid());
    check(Heads.IsValidIndex(HeadIndex));

    // Update the orientation of all modules in the specified head
    FTLHead& HeadData = Heads[HeadIndex];
    for (FTLModule& Module : HeadData.Modules)
    {
        // Reset the offset to identity before applying the new orientation
        Module.Offset = FTransform::Identity;
    }
    for (int32 ModuleIndex = 0; ModuleIndex < HeadData.Modules.Num(); ++ModuleIndex)
    {
        FTLModule& Module = HeadData.Modules[ModuleIndex];
        // Adjust the offset based on the new orientation
        switch (NewOrientation)
        {
            case ETLHeadOrientation::Vertical:
                //TODO: Adjust the transform, not the offset
                Module.Offset.SetLocation(FVector(Module.Offset.GetLocation().X, Module.Offset.GetLocation().Y, ModuleIndex * 70.0f));
                break;
            case ETLHeadOrientation::Horizontal:
                //TODO: Adjust the transform, not the offset
                Module.Offset.SetLocation(FVector(Module.Offset.GetLocation().X, ModuleIndex * 70.0f, Module.Offset.GetLocation().Z));
                break;
            default:
                Module.Offset = FTransform::Identity; // Reset to identity if no valid orientation
                break;
        }
    }
}

FReply STrafficLightToolWidget::OnMoveModuleUpClicked(int32 HeadIndex, int32 ModuleIndex)
{
    OnMoveModuleUp(HeadIndex, ModuleIndex);
    FTLHead& HeadData {Heads[HeadIndex]};
    Rebuild();
    return FReply::Handled();
}

FReply STrafficLightToolWidget::OnMoveModuleDownClicked(int32 HeadIndex, int32 ModuleIndex)
{
    OnMoveModuleDown(HeadIndex, ModuleIndex);
    FTLHead& HeadData {Heads[HeadIndex]};
    Rebuild();
    return FReply::Handled();
}

void STrafficLightToolWidget::OnMoveModuleUp(int32 HeadIndex, int32 ModuleIndex)
{
    check(Heads.IsValidIndex(HeadIndex));
    FTLHead& HeadData = Heads[HeadIndex];

    if (ModuleIndex > 0 && ModuleIndex < HeadData.Modules.Num())
    {
        HeadData.Modules.Swap(ModuleIndex, ModuleIndex - 1);
        Rebuild();
    }
}

void STrafficLightToolWidget::OnMoveModuleDown(int32 HeadIndex, int32 ModuleIndex)
{
    check(Heads.IsValidIndex(HeadIndex));
    FTLHead& HeadData = Heads[HeadIndex];

    if (ModuleIndex >= 0 && ModuleIndex < HeadData.Modules.Num() - 1)
    {
        HeadData.Modules.Swap(ModuleIndex, ModuleIndex + 1);
        Rebuild();
    }
}

void STrafficLightToolWidget::RefreshModuleMeshOptions()
{
    ModuleMeshNameOptionsPerHead.SetNum(Heads.Num());

    for (int32 HeadIdx = 0; HeadIdx < Heads.Num(); ++HeadIdx)
    {
        FTLHead& Head = Heads[HeadIdx];
        ModuleMeshNameOptionsPerHead[HeadIdx].SetNum(Head.Modules.Num());

        for (int32 ModIdx = 0; ModIdx < Head.Modules.Num(); ++ModIdx)
        {
            TArray<UStaticMesh*> MeshOptions = FModuleMeshFactory::GetAllMeshesForModule(Head, Head.Modules[ModIdx]);

            auto& NameList = ModuleMeshNameOptionsPerHead[HeadIdx][ModIdx];
            NameList.Empty();

            for (UStaticMesh* Mesh : MeshOptions)
            {
                if (Mesh)
                {
                    NameList.Add(MakeShared<FString>(Mesh->GetName()));
                }
            }
        }
    }
}
