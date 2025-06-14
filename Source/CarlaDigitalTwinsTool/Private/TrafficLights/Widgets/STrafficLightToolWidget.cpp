#include "TrafficLights/Widgets/STrafficLightToolWidget.h"
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
    FTLModule& Module = Heads[HeadIndex].Modules[ModuleIndex];

    static constexpr float PosMin   = -50.0f, PosMax   =  50.0f;
    static constexpr float RotMin   =   0.0f, RotMax   = 360.0f;
    static constexpr float ScaleMin =   0.1f, ScaleMax =  10.0f;

    return SNew(SBorder)
        .Padding(4)
    [
        SNew(SVerticalBox)

        + SVerticalBox::Slot()
          .AutoHeight()
          .Padding(0,0,0,4)
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            .Text(FText::FromString(
                FString::Printf(TEXT("Module %d"), ModuleIndex)
            ))
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
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
                    return SNew(STextBlock).Text(FText::FromString(*InItem));
                })
                .OnSelectionChanged_Lambda([this, HeadIndex, ModuleIndex](TSharedPtr<FString> NewSel, ESelectInfo::Type) {
                    int32 Choice = LightTypeOptions.IndexOfByPredicate(
                        [&](auto& StrPtr){ return *StrPtr == *NewSel; }
                    );
                    Heads[HeadIndex].Modules[ModuleIndex].LightType =
                        static_cast<ETLLightType>(Choice);
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, HeadIndex, ModuleIndex]() {
                        return FText::FromString(
                            GetLightTypeText(
                                Heads[HeadIndex].Modules[ModuleIndex].LightType
                            )
                        );
                    })
                ]
            ]
        ]

        // — Has Visor Checkbox —
        + SVerticalBox::Slot()
          .AutoHeight()
          .Padding(0,2)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([&Module]() {
                return Module.bHasVisor
                    ? ECheckBoxState::Checked
                    : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([this, HeadIndex, ModuleIndex](ECheckBoxState NewState) {
                Heads[HeadIndex].Modules[ModuleIndex].bHasVisor =
                    (NewState == ECheckBoxState::Checked);
            })
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
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetLocation().X;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V){
                    auto& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FVector L = M.Offset.GetLocation();
                    L.X = V;
                    M.Offset.SetLocation(L);
                    if (PreviewViewport.IsValid())
                {
                    UpdateModuleMeshesInViewport(HeadIndex);
                }
                })
            ]

            // Slider
            + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2,0)
            [
                SNew(SSlider)
                .MinValue(PosMin).MaxValue(PosMax)
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetLocation().X;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V){
                    auto& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FVector L = M.Offset.GetLocation();
                    L.X = V;
                    M.Offset.SetLocation(L);
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
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
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetLocation().Y;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V){
                    auto& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FVector L = M.Offset.GetLocation();
                    L.Y = V;
                    M.Offset.SetLocation(L);
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
                    }
                })
            ]
            + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2,0)
            [
                SNew(SSlider)
                .MinValue(PosMin).MaxValue(PosMax)
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetLocation().Y;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V){
                    auto& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FVector L = M.Offset.GetLocation();
                    L.Y = V;
                    M.Offset.SetLocation(L);
                    if (PreviewViewport.IsValid())
                {
                    UpdateModuleMeshesInViewport(HeadIndex);
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
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetLocation().Z;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V){
                    auto& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FVector L = M.Offset.GetLocation();
                    L.Z = V;
                    M.Offset.SetLocation(L);
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
                    }
                })
            ]
            + SHorizontalBox::Slot().FillWidth(0.7f).Padding(2,0)
            [
                SNew(SSlider)
                .MinValue(PosMin).MaxValue(PosMax)
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetLocation().Z;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V){
                    auto& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FVector L = M.Offset.GetLocation();
                    L.Z = V;
                    M.Offset.SetLocation(L);
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
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
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.Rotator().Pitch;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V) {
                    FTLModule& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FRotator R = M.Offset.Rotator();
                    R.Pitch = V;
                    M.Offset.SetRotation(FQuat(R));
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
                    }
                })
            ]

            // Yaw
            + SHorizontalBox::Slot().FillWidth(1).Padding(2,0)
            [
                SNew(SNumericEntryBox<float>)
                .MinValue(RotMin).MaxValue(RotMax)
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.Rotator().Yaw;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V) {
                    FTLModule& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FRotator R = M.Offset.Rotator();
                    R.Yaw = V;
                    M.Offset.SetRotation(FQuat(R));
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
                    }
                })
            ]

            // Roll
            + SHorizontalBox::Slot().FillWidth(1).Padding(2,0)
            [
                SNew(SNumericEntryBox<float>)
                .MinValue(RotMin).MaxValue(RotMax)
                .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                    return Heads[HeadIndex].Modules[ModuleIndex].Offset.Rotator().Roll;
                })
                .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V) {
                    FTLModule& M = Heads[HeadIndex].Modules[ModuleIndex];
                    FRotator R = M.Offset.Rotator();
                    R.Roll = V;
                    M.Offset.SetRotation(FQuat(R));
                    if (PreviewViewport.IsValid())
                    {
                        UpdateModuleMeshesInViewport(HeadIndex);
                    }
                })
            ]
        ]

        //----------------------------------------------------------------
        // Offset Scale (uniform)
        //----------------------------------------------------------------
        + SVerticalBox::Slot().AutoHeight().Padding(0,2)
        [
            SNew(STextBlock).Text(FText::FromString("Offset Scale"))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0,2)
        [
            SNew(SSlider)
            .MinValue(ScaleMin).MaxValue(ScaleMax)
            .Value_Lambda([this, HeadIndex, ModuleIndex]() {
                return Heads[HeadIndex].Modules[ModuleIndex].Offset.GetScale3D().X;
            })
            .OnValueChanged_Lambda([this, HeadIndex, ModuleIndex](float V) {
                FTLModule& M = Heads[HeadIndex].Modules[ModuleIndex];
                M.Offset.SetScale3D(FVector(V));
                if (PreviewViewport.IsValid())
                {
                    UpdateModuleMeshesInViewport(HeadIndex);
                }
            })
        ]

        // Delete Module button
        + SVerticalBox::Slot().AutoHeight().Padding(0,2)
        [
            SNew(SButton)
            .Text(FText::FromString("Delete Module"))
            .OnClicked(this, &STrafficLightToolWidget::OnDeleteModuleClicked, HeadIndex, ModuleIndex)
        ]
    ];
}

TSharedRef<SWidget> STrafficLightToolWidget::BuildModulesSection(int32 HeadIndex)
{
    TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

     Container->AddSlot()
    .AutoHeight()
    .Padding(2,4)
    [
        SNew(STextBlock)
        .Text(FText::FromString("Modules"))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
    ];

    for (int32 ModuleIndex = 0; ModuleIndex < Heads[HeadIndex].Modules.Num(); ++ModuleIndex)
    {
        Container->AddSlot()
        .AutoHeight()
        .Padding(4,2)
        [
            BuildModuleEntry(HeadIndex, ModuleIndex)
        ];
    }

    return Container;
}

FReply STrafficLightToolWidget::OnAddHeadClicked()
{
    // Create new head data
    FTLHead NewHead;
    NewHead.HeadID = FGuid::NewGuid();
    NewHead.Transform = FTransform::Identity;
    NewHead.Offset = FTransform::Identity;

    const int32 Index {Heads.Add(NewHead)};
    OnAddModuleClicked(Index);
    RefreshHeadList();
    return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeleteHeadClicked(int32 Index)
{
    check(Heads.IsValidIndex(Index));
    for (int32 ModuleIndex = 0; ModuleIndex < Heads[Index].Modules.Num(); ++ModuleIndex)
    {
        Heads[Index].Modules.RemoveAt(ModuleIndex);
        PreviewViewport->RemoveModuleMeshesForHead(ModuleIndex);
    }
    Heads.RemoveAt(Index);
    RefreshHeadList();
    return FReply::Handled();
}

void STrafficLightToolWidget::RefreshHeadList()
{
    HeadListContainer->ClearChildren();

    for (int32 i = 0; i < Heads.Num(); ++i)
    {
        HeadListContainer->AddSlot().AutoHeight().Padding(5)
        [
            CreateHeadEntry(i)
        ];
    }
}

TSharedRef<SWidget> STrafficLightToolWidget::CreateHeadEntry(int32 Index)
{
    const FTLHead& Head = Heads[Index];

    static constexpr float PosMin   { -50.0f };
    static constexpr float PosMax   {  50.0f };
    static constexpr float RotMin   {   0.0f };
    static constexpr float RotMax   { 360.0f };
    static constexpr float ScaleMin {   0.1f };
    static constexpr float ScaleMax {  10.0f };

    return SNew(SBorder)
        .BorderBackgroundColor(FLinearColor{ 0.15f, 0.15f, 0.15f })
        .Padding(8)
    [
        SNew(SVerticalBox)

        // Header
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,5)
        [
            SNew(STextBlock)
            .Text(FText::FromString(FString::Printf(TEXT("Head #%d"), Index)))
        ]

        // Head Style
        + SVerticalBox::Slot().AutoHeight().Padding(2)
        [ SNew(STextBlock).Text(FText::FromString("Head Style")) ]
        + SVerticalBox::Slot().AutoHeight().Padding(2)
        [
            SNew(SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&HeadStyleOptions)
            .InitiallySelectedItem(HeadStyleOptions[(int32)Head.HeadStyle])
            .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
            {
                return SNew(STextBlock).Text(FText::FromString(*InItem));
            })
            .OnSelectionChanged_Lambda([this,Index](TSharedPtr<FString> New, ESelectInfo::Type)
            {
                int32 Choice = HeadStyleOptions.IndexOfByPredicate([&](auto& S){ return S == New; });
                Heads[Index].HeadStyle = static_cast<ETLHeadStyle>(Choice);
                PreviewViewport->SetHeadStyle(Index, Heads[Index].HeadStyle);
            })
            [
                SNew(STextBlock)
                .Text_Lambda([this,Index](){ return FText::FromString(*HeadStyleOptions[(int32)Heads[Index].HeadStyle]); })
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
                int32 Choice = HeadAttachmentOptions.IndexOfByPredicate([&](auto& S){ return S == New; });
                Heads[Index].Attachment = static_cast<ETLHeadAttachment>(Choice);
            })
            [
                SNew(STextBlock)
                .Text_Lambda([this,Index](){ return FText::FromString(*HeadAttachmentOptions[(int32)Heads[Index].Attachment]); })
            ]
        ]

        // Orientation
        + SVerticalBox::Slot().AutoHeight().Padding(2)
        [ SNew(STextBlock).Text(FText::FromString("Orientation")) ]
        + SVerticalBox::Slot().AutoHeight().Padding(2)
        [
            SNew(SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&HeadOrientationOptions)
            .InitiallySelectedItem(HeadOrientationOptions[(int32)Head.Orientation])
            .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
            {
                return SNew(STextBlock).Text(FText::FromString(*InItem));
            })
            .OnSelectionChanged_Lambda([this,Index](TSharedPtr<FString> New, ESelectInfo::Type)
            {
                int32 Choice = HeadOrientationOptions.IndexOfByPredicate([&](auto& S){ return S == New; });
                Heads[Index].Orientation = static_cast<ETLHeadOrientation>(Choice);
                ChangeModulesOrientation(Index, Heads[Index].Orientation);
                UpdateModuleMeshesInViewport(Index);
                RefreshHeadList();
            })
            [
                SNew(STextBlock)
                .Text_Lambda([this,Index](){ return FText::FromString(*HeadOrientationOptions[(int32)Heads[Index].Orientation]); })
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
                    // Spawnea el cubo de backplate
                    PreviewViewport->AddBackplateMesh(Index);
                }
                else
                {
                    // Destruye el cubo de backplate
                    PreviewViewport->RemoveBackplateMesh(Index);
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
                    FVector Loc{ Heads[Index].Transform.GetLocation() };
                    Loc.X = V;
                    Heads[Index].Transform.SetLocation(Loc);
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() {
                    return Heads[Index].Transform.GetLocation().Y;
                })
                .OnValueChanged_Lambda([this,Index](float V){
                    FVector Loc{ Heads[Index].Transform.GetLocation() };
                    Loc.Y = V;
                    Heads[Index].Transform.SetLocation(Loc);
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() {
                    return Heads[Index].Transform.GetLocation().Z;
                })
                .OnValueChanged_Lambda([this,Index](float V){
                    FVector Loc{ Heads[Index].Transform.GetLocation() };
                    Loc.Z = V;
                    Heads[Index].Transform.SetLocation(Loc);
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
                    FVector Off{ Heads[Index].Offset.GetLocation() };
                    Off.X = V;
                    Heads[Index].Offset.SetLocation(Off);
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() {
                    return Heads[Index].Offset.GetLocation().Y;
                })
                .OnValueChanged_Lambda([this,Index](float V){
                    FVector Off{ Heads[Index].Offset.GetLocation() };
                    Off.Y = V;
                    Heads[Index].Offset.SetLocation(Off);
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() {
                    return Heads[Index].Offset.GetLocation().Z;
                })
                .OnValueChanged_Lambda([this,Index](float V){
                    FVector Off{ Heads[Index].Offset.GetLocation() };
                    Off.Z = V;
                    Heads[Index].Offset.SetLocation(Off);
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
                    FRotator R{ Heads[Index].Offset.Rotator() };
                    R.Pitch = V;
                    Heads[Index].Offset.SetRotation(FQuat{ R });
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() {
                    return Heads[Index].Offset.Rotator().Yaw;
                })
                .OnValueChanged_Lambda([this,Index](float V){
                    FRotator R{ Heads[Index].Offset.Rotator() };
                    R.Yaw = V;
                    Heads[Index].Offset.SetRotation(FQuat{ R });
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() {
                    return Heads[Index].Offset.Rotator().Roll;
                })
                .OnValueChanged_Lambda([this,Index](float V){
                    FRotator R{ Heads[Index].Offset.Rotator() };
                    R.Roll = V;
                    Heads[Index].Offset.SetRotation(FQuat{ R });
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
                    FVector S{ Heads[Index].Offset.GetScale3D() };
                    S.X = V;
                    Heads[Index].Offset.SetScale3D(S);
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() { return Heads[Index].Offset.GetScale3D().Y; })
                .OnValueChanged_Lambda([this,Index](float V){
                    FVector S{ Heads[Index].Offset.GetScale3D() };
                    S.Y = V;
                    Heads[Index].Offset.SetScale3D(S);
                })
            ]

            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SNumericEntryBox<float>)
                .Value_Lambda([this,Index]() { return Heads[Index].Offset.GetScale3D().Z; })
                .OnValueChanged_Lambda([this,Index](float V){
                    FVector S{ Heads[Index].Offset.GetScale3D() };
                    S.Z = V;
                    Heads[Index].Offset.SetScale3D(S);
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
                BuildModulesSection(Index)
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
    ];
}

void STrafficLightToolWidget::RebuildModuleChain(FTLHead& Head)
{
    if (Head.Modules.Num() == 0)
    {
        return;
    }

    Head.Modules[0].Transform = FTransform::Identity;
    if (Head.Modules[0].ModuleMeshComponent)
    {
        Head.Modules[0].ModuleMeshComponent->SetRelativeTransform(Head.Modules[0].Transform * Head.Modules[0].Offset);
    }

    for (int32 i {1}; i < Head.Modules.Num(); ++i)
    {
        const FTLModule& PrevModule {Head.Modules[i - 1]};
        FTLModule& CurrModule {Head.Modules[i]};

        const FName EndSocketName {FName("Socket2")};
        const FName BeginSocketName {FName("Socket1")};

        if (PrevModule.ModuleMeshComponent && CurrModule.ModuleMeshComponent)
        {
            if (PrevModule.ModuleMeshComponent->DoesSocketExist(EndSocketName) && CurrModule.ModuleMeshComponent->DoesSocketExist(BeginSocketName))
            {
                const FTransform EndSocketLocal {PrevModule.ModuleMeshComponent->GetSocketTransform(EndSocketName, RTS_Component)};
                const FTransform EndSocketWorldRelativeToHead {PrevModule.Transform * EndSocketLocal};
                const FTransform BeginSocketLocal {CurrModule.ModuleMeshComponent->GetSocketTransform(BeginSocketName, RTS_Component)};
                const FTransform SnapDelta {EndSocketWorldRelativeToHead * BeginSocketLocal.Inverse()};

                CurrModule.Transform = SnapDelta;
                CurrModule.ModuleMeshComponent->SetRelativeTransform(CurrModule.Transform * CurrModule.Offset);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("One of the sockets is missing in module %d"), i);
            }
        }
    }
}

FReply STrafficLightToolWidget::OnAddModuleClicked(int32 HeadIndex)
{
    check(Heads.IsValidIndex(HeadIndex));

    FTLHead& HeadData = Heads[HeadIndex];

    FTLModule NewModule;
    NewModule.ModuleID = FGuid::NewGuid();
    NewModule.Offset = FTransform::Identity;

    NewModule.ModuleMesh = Cast<UStaticMesh>(StaticLoadObject(
        UStaticMesh::StaticClass(), nullptr,
        TEXT("/CarlaDigitalTwinsTool/Carla/Static/TrafficLight/TrafficLights2025/TrafficLights/SM_TrafficLights_Black_Module_01.SM_TrafficLights_Black_Module_01")
    ));

    if (!NewModule.ModuleMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load module mesh."));
        return FReply::Handled();
    }

    HeadData.Modules.Add(NewModule);
    FTLModule& StoredModule {HeadData.Modules.Last()};
    StoredModule.ModuleMeshComponent = PreviewViewport->AddModuleMesh(HeadData, StoredModule);

    RebuildModuleChain(HeadData);
    RefreshHeadList();

    return FReply::Handled();
}

FReply STrafficLightToolWidget::OnDeleteModuleClicked(int32 HeadIndex, int32 ModuleIndex)
{
    check(Heads.IsValidIndex(HeadIndex));
    check(Heads[HeadIndex].Modules.IsValidIndex(ModuleIndex));

    Heads[HeadIndex].Modules.RemoveAt(ModuleIndex);
    PreviewViewport->RemoveModuleMeshForHead(HeadIndex, ModuleIndex);
    ChangeModulesOrientation(HeadIndex, Heads[HeadIndex].Orientation);
    UpdateModuleMeshesInViewport(HeadIndex);
    RefreshHeadList();
    return FReply::Handled();
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

void STrafficLightToolWidget::UpdateModuleMeshesInViewport(int32 HeadIndex)
{
    check(PreviewViewport.IsValid());
    check(Heads.IsValidIndex(HeadIndex));

    PreviewViewport->RemoveModuleMeshesForHead(HeadIndex);
    const FTLHead& HeadData = Heads[HeadIndex];
    for (const FTLModule& Mod : HeadData.Modules)
    {
        PreviewViewport->AddModuleMesh(HeadData, Mod);
    }
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
