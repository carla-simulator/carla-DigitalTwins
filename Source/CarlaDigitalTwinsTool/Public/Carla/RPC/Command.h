// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once


#include "Carla/MsgPackAdaptors.h"
#include "Carla/Geom/Transform.h"
#include "Carla/RPC/ActorDescription.h"
#include "Carla/RPC/AttachmentType.h"
#include "Carla/RPC/ActorId.h"
#include "Carla/RPC/TrafficLightState.h"
#include "Carla/RPC/VehicleAckermannControl.h"
#include "Carla/RPC/VehicleControl.h"
#include "Carla/RPC/VehiclePhysicsControl.h"
#include "Carla/RPC/VehicleLightState.h"
#include "Carla/RPC/WalkerControl.h"

#include <string>

#include <Carla/disable-ue4-macros.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4583)
#pragma warning(disable:4582)
#include <boost/variant2/variant.hpp>
#pragma warning(pop)
#else
#include <boost/variant2/variant.hpp>
#endif
#include <Carla/enable-ue4-macros.h>
namespace carla {

namespace traffic_manager {
  class TrafficManager;
}

namespace ctm = carla::traffic_manager;

namespace rpc {

  class Command {
  private:

    template <typename T>
    struct CommandBase {
      operator Command() const {
        return Command{*static_cast<const T *>(this)};
      }
    };

  public:

    struct SpawnActor : CommandBase<SpawnActor> {
      SpawnActor() = default;
      SpawnActor(ActorDescription description, const geom::Transform &transform)
        : description(std::move(description)),
          transform(transform) {}
      SpawnActor(ActorDescription description, const geom::Transform &transform, ActorId parent)
        : description(std::move(description)),
          transform(transform),
          parent(parent) {}
      SpawnActor(ActorDescription description, const geom::Transform &transform, ActorId parent, AttachmentType attachment_type, const std::string& bone)
        : description(std::move(description)),
          transform(transform),
          parent(parent),
          attachment_type(attachment_type),
          socket_name(bone) {}
      ActorDescription description;
      geom::Transform transform;
      boost::optional<ActorId> parent;
      AttachmentType attachment_type;
      std::string socket_name;
      std::vector<Command> do_after;
      ;
    };

    struct DestroyActor : CommandBase<DestroyActor> {
      DestroyActor() = default;
      DestroyActor(ActorId id)
        : actor(id) {}
      ActorId actor;
      ;
    };

    struct ApplyVehicleControl : CommandBase<ApplyVehicleControl> {
      ApplyVehicleControl() = default;
      ApplyVehicleControl(ActorId id, const VehicleControl &value)
        : actor(id),
          control(value) {}
      ActorId actor;
      VehicleControl control;
      ;
    };

    struct ApplyVehicleAckermannControl : CommandBase<ApplyVehicleAckermannControl> {
      ApplyVehicleAckermannControl() = default;
      ApplyVehicleAckermannControl(ActorId id, const VehicleAckermannControl &value)
        : actor(id),
          control(value) {}
      ActorId actor;
      VehicleAckermannControl control;
      ;
    };

    struct ApplyWalkerControl : CommandBase<ApplyWalkerControl> {
      ApplyWalkerControl() = default;
      ApplyWalkerControl(ActorId id, const WalkerControl &value)
        : actor(id),
          control(value) {}
      ActorId actor;
      WalkerControl control;
      ;
    };

    struct ApplyVehiclePhysicsControl : CommandBase<ApplyVehiclePhysicsControl> {
      ApplyVehiclePhysicsControl() = default;
      ApplyVehiclePhysicsControl(ActorId id, const VehiclePhysicsControl &value)
        : actor(id),
          physics_control(value) {}
      ActorId actor;
      VehiclePhysicsControl physics_control;
      ;
    };

    struct ApplyTransform : CommandBase<ApplyTransform> {
      ApplyTransform() = default;
      ApplyTransform(ActorId id, const geom::Transform &value)
        : actor(id),
          transform(value) {}
      ActorId actor;
      geom::Transform transform;
      ;
    };

    struct ApplyLocation : CommandBase<ApplyLocation> {
      ApplyLocation() = default;
      ApplyLocation(ActorId id, const geom::Location &value)
        : actor(id),
          location(value) {}
      ActorId actor;
      geom::Location location;
      ;
    };

    struct ApplyWalkerState : CommandBase<ApplyWalkerState> {
      ApplyWalkerState() = default;
      ApplyWalkerState(ActorId id, const geom::Transform &value, const float speed) : actor(id), transform(value), speed(speed) {}
      ActorId actor;
      geom::Transform transform;
      float speed;
      ;
    };

    struct ApplyTargetVelocity : CommandBase<ApplyTargetVelocity> {
      ApplyTargetVelocity() = default;
      ApplyTargetVelocity(ActorId id, const geom::Vector3D &value)
        : actor(id),
          velocity(value) {}
      ActorId actor;
      geom::Vector3D velocity;
      ;
    };

    struct ApplyTargetAngularVelocity : CommandBase<ApplyTargetAngularVelocity> {
      ApplyTargetAngularVelocity() = default;
      ApplyTargetAngularVelocity(ActorId id, const geom::Vector3D &value)
        : actor(id),
          angular_velocity(value) {}
      ActorId actor;
      geom::Vector3D angular_velocity;
      ;
    };

    struct ApplyImpulse : CommandBase<ApplyImpulse> {
      ApplyImpulse() = default;
      ApplyImpulse(ActorId id, const geom::Vector3D &value)
        : actor(id),
          impulse(value) {}
      ActorId actor;
      geom::Vector3D impulse;
      ;
    };

    struct ApplyForce : CommandBase<ApplyForce> {
      ApplyForce() = default;
      ApplyForce(ActorId id, const geom::Vector3D &value)
        : actor(id),
          force(value) {}
      ActorId actor;
      geom::Vector3D force;
      ;
    };

    struct ApplyAngularImpulse : CommandBase<ApplyAngularImpulse> {
      ApplyAngularImpulse() = default;
      ApplyAngularImpulse(ActorId id, const geom::Vector3D &value)
        : actor(id),
          impulse(value) {}
      ActorId actor;
      geom::Vector3D impulse;
      ;
    };

    struct ApplyTorque : CommandBase<ApplyTorque> {
      ApplyTorque() = default;
      ApplyTorque(ActorId id, const geom::Vector3D &value)
        : actor(id),
          torque(value) {}
      ActorId actor;
      geom::Vector3D torque;
      ;
    };

    struct SetSimulatePhysics : CommandBase<SetSimulatePhysics> {
      SetSimulatePhysics() = default;
      SetSimulatePhysics(ActorId id, bool value)
        : actor(id),
          enabled(value) {}
      ActorId actor;
      bool enabled;
      ;
    };

    struct SetEnableGravity : CommandBase<SetEnableGravity> {
      SetEnableGravity() = default;
      SetEnableGravity(ActorId id, bool value)
        : actor(id),
          enabled(value) {}
      ActorId actor;
      bool enabled;
      ;
    };

    struct SetAutopilot : CommandBase<SetAutopilot> {
      SetAutopilot() = default;
      SetAutopilot(
          ActorId id,
          bool value,
          uint16_t tm_port)
        : actor(id),
          enabled(value),
          tm_port(tm_port) {}
      ActorId actor;
      bool enabled;
      uint16_t tm_port;
      ;
    };

    struct ShowDebugTelemetry : CommandBase<ShowDebugTelemetry> {
      ShowDebugTelemetry() = default;
      ShowDebugTelemetry(
          ActorId id,
          bool value)
        : actor(id),
          enabled(value) {}
      ActorId actor;
      bool enabled;
      ;
    };

    struct SetVehicleLightState : CommandBase<SetVehicleLightState> {
      SetVehicleLightState() = default;
      SetVehicleLightState(
          ActorId id,
          VehicleLightState::flag_type value)
        : actor(id),
          light_state(value) {}
      ActorId actor;
      VehicleLightState::flag_type light_state;
      ;
    };

    struct ConsoleCommand : CommandBase<ConsoleCommand> {
      ConsoleCommand() = default;
      ConsoleCommand(std::string cmd) : cmd(cmd) {}
      std::string cmd;
      ;
    };

    struct SetTrafficLightState : CommandBase<SetTrafficLightState> {
      SetTrafficLightState() = default;
      SetTrafficLightState(
          ActorId id,
          rpc::TrafficLightState state)
        : actor(id),
          traffic_light_state(state) {}
      ActorId actor;
      rpc::TrafficLightState traffic_light_state;
      ;
    };

    using CommandType = boost::variant2::variant<
        SpawnActor,
        DestroyActor,
        ApplyVehicleControl,
        ApplyVehicleAckermannControl,
        ApplyWalkerControl,
        ApplyVehiclePhysicsControl,
        ApplyTransform,
        ApplyWalkerState,
        ApplyTargetVelocity,
        ApplyTargetAngularVelocity,
        ApplyImpulse,
        ApplyForce,
        ApplyAngularImpulse,
        ApplyTorque,
        SetSimulatePhysics,
        SetEnableGravity,
        SetAutopilot,
        ShowDebugTelemetry,
        SetVehicleLightState,
        ApplyLocation,
        ConsoleCommand,
        SetTrafficLightState>;

    CommandType command;

    ;
  };

} // namespace rpc
} // namespace carla
