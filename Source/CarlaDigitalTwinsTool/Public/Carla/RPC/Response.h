// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include <Carla/disable-ue4-macros.h>
#include <boost/optional.hpp>

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


#include <string>

namespace carla {
namespace rpc {

  class ResponseError {
  public:

    ResponseError() = default;

    explicit ResponseError(std::string message)
      : _what(std::move(message)) {}

    const std::string &What() const {
      return _what;
    }

    

  private:

    /// @todo Needs initialization, empty strings end up calling memcpy on a
    /// nullptr. Possibly a bug in MsgPack but could also be our specialization
    /// for variants
    std::string _what{"unknown error"};
  };

  template <typename T>
  class Response {
  public:

    using value_type = T;

    using error_type = ResponseError;

    Response() = default;

    template <typename TValue>
    Response(TValue &&value) : _data(std::forward<TValue>(value)) {}

    template <typename TValue>
    void Reset(TValue &&value) {
      _data = std::forward<TValue>(value);
    }

    bool HasError() const {
      return _data.index() == 0;
    }

    template <typename... Ts>
    void SetError(Ts &&... args) {
      _data = error_type(std::forward<Ts>(args)...);
    }

    const error_type &GetError() const {
      DEBUG_ASSERT(HasError());
      return boost::variant2::get<error_type>(_data);
    }

    value_type &Get() {
      DEBUG_ASSERT(!HasError());
      return boost::variant2::get<value_type>(_data);
    }

    const value_type &Get() const {
      DEBUG_ASSERT(!HasError());
      return boost::variant2::get<value_type>(_data);
    }

    operator bool() const {
      return !HasError();
    }

    

  private:

    boost::variant2::variant<error_type, value_type> _data;
  };

  template <>
  class Response<void> {
  public:

    using value_type = void;

    using error_type = ResponseError;

    static Response Success() {
      return success_flag{};
    }

    Response() : _data(error_type{}) {}

    Response(ResponseError error) : _data(std::move(error)) {}

    bool HasError() const {
      return _data.has_value();
    }

    template <typename... Ts>
    void SetError(Ts &&... args) {
      _data = error_type(std::forward<Ts>(args)...);
    }

    const error_type &GetError() const {
      DEBUG_ASSERT(HasError());
      return *_data;
    }

    operator bool() const {
      return !HasError();
    }

    

  private:

    struct success_flag {};

    Response(success_flag) {}

    boost::optional<error_type> _data;
  };

} // namespace rpc
} // namespace carla
