// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once



#include <cstdint>

namespace carla {
namespace rpc {

  enum class QualityLevel : uint8_t {
    Low,
    // Medium,
    // High,
    Epic,

    SIZE,
    INVALID
  };

} // namespace rpc
} // namespace carla

MSGPACK_ADD_ENUM(carla::rpc::QualityLevel);
