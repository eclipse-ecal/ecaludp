/********************************************************************************
 * Copyright (c) 2024 Continental Corporation
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * 
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "socket_builder_npcap.h"

#include <memory>
#include <array>
#include <stdexcept>

#include <asio.hpp>
#include <ecaludp/socket_npcap.h>

#include "receiver_parameters.h"

namespace SocketBuilderNpcap
{
  std::shared_ptr<ecaludp::SocketNpcap> CreateReceiveSocket(const ReceiverParameters& parameters)
  {
    auto socket = std::make_shared<ecaludp::SocketNpcap>(std::array<char, 4>{'E', 'C', 'A', 'L'});
    
    asio::ip::address ip_address {};
    {
      asio::error_code ec;
      ip_address = asio::ip::make_address(parameters.ip, ec);
      if (ec)
      {
        throw std::runtime_error("Invalid IP address: " + parameters.ip);
      }
    }

    // Set receive buffer size
    if (parameters.buffer_size > 0)
    {
      asio::socket_base::receive_buffer_size option(parameters.buffer_size);
      bool success = socket->set_receive_buffer_size(parameters.buffer_size);
      if (!success)
      {
        throw std::runtime_error("Failed to set receive buffer size");
      }
    }

    asio::ip::udp::endpoint destination(ip_address, parameters.port);

    {
      bool success = socket->bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
      if (!success)
      {
        throw std::runtime_error("Failed to bind socket");
      }
    }

    return socket;
  }
}
