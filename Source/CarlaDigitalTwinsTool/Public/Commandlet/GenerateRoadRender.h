// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/ObjectLibrary.h"
#include "Commandlets/Commandlet.h"
#include "GenerateRoadRender.generated.h"

// Each commandlet should generate only 1 Tile

DECLARE_LOG_CATEGORY_EXTERN(LogCarlaToolsMapGenerateRoadRender, Log, All);

class UOpenDriveToMap;

UCLASS()
class CARLADIGITALTWINSTOOL_API UGenerateRoadRender
  : public UCommandlet
{
  GENERATED_BODY()

public:

  /// Default constructor.
  UGenerateRoadRender();
  UGenerateRoadRender(const FObjectInitializer &);

#if WITH_EDITORONLY_DATA

  virtual int32 Main(const FString &Params) override;

#endif // WITH_EDITORONLY_DATA

  UPROPERTY()
  UOpenDriveToMap* OpenDriveMap;

  UPROPERTY()
  UClass* OpenDriveClass;
  
};
