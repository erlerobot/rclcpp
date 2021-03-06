// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#include "rclcpp/client.hpp"

#include <cstdio>
#include <string>

#include "rmw/rmw.h"

using rclcpp::client::ClientBase;

ClientBase::ClientBase(
  std::shared_ptr<rmw_node_t> node_handle,
  rmw_client_t * client_handle,
  const std::string & service_name)
: node_handle_(node_handle), client_handle_(client_handle), service_name_(service_name)
{}

ClientBase::~ClientBase()
{
  if (client_handle_) {
    if (rmw_destroy_client(client_handle_) != RMW_RET_OK) {
      fprintf(stderr,
        "Error in destruction of rmw client handle: %s\n", rmw_get_error_string_safe());
    }
  }
}

const std::string &
ClientBase::get_service_name() const
{
  return this->service_name_;
}

const rmw_client_t *
ClientBase::get_client_handle() const
{
  return this->client_handle_;
}
