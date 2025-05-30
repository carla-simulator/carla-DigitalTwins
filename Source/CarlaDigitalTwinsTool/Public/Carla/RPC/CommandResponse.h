// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Carla/RPC/ActorId.h"
#include "Carla/RPC/Response.h"

namespace carla {
namespace rpc {

  using CommandResponse = Response<ActorId>;

} // namespace rpc
} // namespace carla
