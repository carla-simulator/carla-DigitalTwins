// Copyright (c) 2020 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Actor/ProceduralMeshActor.h"

AProceduralMeshActor::AProceduralMeshActor()
{
  PrimaryActorTick.bCanEverTick = false;
  MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RootComponent"));
  RootComponent = MeshComponent;
}
