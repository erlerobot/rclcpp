// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RCLCPP__SERVICE_HPP_
#define RCLCPP__SERVICE_HPP_

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "rcl/error_handling.h"
#include "rcl/service.h"

#include "rclcpp/any_service_callback.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp/type_support_decl.hpp"
#include "rclcpp/visibility_control.hpp"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

namespace rclcpp
{
namespace service
{

class ServiceBase
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS_NOT_COPYABLE(ServiceBase);

  RCLCPP_PUBLIC
  ServiceBase(
    std::shared_ptr<rcl_node_t> node_handle,
    const std::string service_name);

  RCLCPP_PUBLIC
  virtual ~ServiceBase();

  RCLCPP_PUBLIC
  std::string
  get_service_name();

  RCLCPP_PUBLIC
  const rcl_service_t *
  get_service_handle();

  virtual std::shared_ptr<void> create_request() = 0;
  virtual std::shared_ptr<rmw_request_id_t> create_request_header() = 0;
  virtual void handle_request(
    std::shared_ptr<rmw_request_id_t> request_header,
    std::shared_ptr<void> request) = 0;

protected:
  RCLCPP_DISABLE_COPY(ServiceBase);

  std::shared_ptr<rcl_node_t> node_handle_;

  rcl_service_t service_handle_ = rcl_get_zero_initialized_service();
  std::string service_name_;
};

using any_service_callback::AnyServiceCallback;


// TODO Store function etc.
template<typename RequestT, typename ResponseT>
class ServicePattern : public ServiceBase
{
public:
  using CallbackType = std::function<
      void(
        const std::shared_ptr<RequestT>,
        std::shared_ptr<ResponseT>)>;

  using CallbackWithHeaderType = std::function<
      void(
        const std::shared_ptr<rmw_request_id_t>,
        const std::shared_ptr<RequestT>,
        std::shared_ptr<ResponseT>)>;
  // RCLCPP_SMART_PTR_DEFINITIONS(Service);

  using SendResponseFunctionT = std::function<void(std::shared_ptr<rmw_request_id_t>, std::shared_ptr<ResponseT>)>;

  // TODO
  ServicePattern(
    AnyServiceCallback<RequestT, ResponseT> any_callback)
  : any_callback_(any_callback)
  {
  }

  ServicePattern() = delete;

  std::shared_ptr<void> create_request()
  {
    return std::shared_ptr<void>(new RequestT());
  }

  std::shared_ptr<rmw_request_id_t> create_request_header()
  {
    // TODO(wjwwood): This should probably use rmw_request_id's allocator.
    //                (since it is a C type)
    return std::shared_ptr<rmw_request_id_t>(new rmw_request_id_t);
  }

  void handle_request(std::shared_ptr<rmw_request_id_t> request_header,
    std::shared_ptr<void> request)
  {
    auto typed_request = std::static_pointer_cast<RequestT>(request);
    auto response = std::shared_ptr<ResponseT>(new ResponseT);
    any_callback_.dispatch(request_header, typed_request, response);
    send_response_function(request_header, response);
  }

protected:
  SendResponseFunctionT send_response_function_;

private:
  RCLCPP_DISABLE_COPY(ServicePattern);

  AnyServiceCallback<RequestT, ResponseT> any_callback_;
};

template<typename ServiceT>
class Service : public ServicePattern<typename ServiceT::Request, typename ServiceT::Response:
{
  using ServicePatternT = ServicePattern<typename ServiceT::Request::SharedPtr, typename ServiceT::Response::SharedPtr>;
public:
  // TODO Also call ServicePattern constructor.
  Service(
    std::shared_ptr<rcl_node_t> node_handle,
    const std::string & service_name,
    AnyServiceCallback<typename ServiceT::Request, typename ServiceT::Response> any_callback,
    rcl_service_options_t & service_options)
  : ServiceBase(node_handle, service_name), ServicePatternT(any_callback)
  {
    using rosidl_generator_cpp::get_service_type_support_handle;
    auto service_type_support_handle = get_service_type_support_handle<ServiceT>();

    if (rcl_service_init(
        &service_handle_, node_handle.get(), service_type_support_handle, service_name.c_str(),
        &service_options) != RCL_RET_OK)
    {
      throw std::runtime_error(
              std::string("could not create service: ") +
              rcl_get_error_string_safe());
    }
  }

  virtual ~Service()
  {
    if (rcl_service_fini(&service_handle_, node_handle_.get()) != RCL_RET_OK) {
      std::stringstream ss;
      ss << "Error in destruction of rcl service_handle_ handle: " <<
        rcl_get_error_string_safe() << '\n';
      (std::cerr << ss.str()).flush();
    }
  }

  RCLCPP_SMART_PTR_DEFINITIONS(Service);
  void send_response(
    std::shared_ptr<rmw_request_id_t> req_id,
    std::shared_ptr<typename ServiceT::Response> response)
  {
    rcl_ret_t status = rcl_send_response(get_service_handle(), req_id.get(), response.get());

    if (status != RCL_RET_OK) {
      // *INDENT-OFF* (prevent uncrustify from making unecessary indents here)
      throw std::runtime_error(
        std::string("failed to send response: ") + rcl_get_error_string_safe());
      // *INDENT-ON*
    }
  }

};

}  // namespace service
}  // namespace rclcpp

#endif  // RCLCPP__SERVICE_HPP_
