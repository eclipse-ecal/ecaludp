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

    // only v4 is supported right now
    if (!ip_address.is_v4())
    {
      throw std::runtime_error("Only IPv4 is supported");
    }

    // Set receive buffer size
    if (parameters.buffer_size > 0)
    {
      const bool success = socket->set_receive_buffer_size(parameters.buffer_size);
      if (!success)
      {
        throw std::runtime_error("Failed to set receive buffer size");
      }
    }


    if (ip_address.is_multicast())
    {
      socket->set_multicast_loopback_enabled(true);

      // "Bind" multicast address
      {
        const asio::ip::udp::endpoint bind_endpoint = asio::ip::udp::endpoint(asio::ip::address_v4(), parameters.port);
        const bool success = socket->bind(bind_endpoint);
        if (!success)
        {
          throw std::runtime_error("Failed to bind socket");
        }
      }

      {
        const bool success = socket->join_multicast_group(ip_address.to_v4());
        if (!success)
        {
          throw std::runtime_error("Failed to join multicast group");
        }
      }
    }
    else
    {
      const asio::ip::udp::endpoint destination(ip_address, parameters.port);
      const bool success = socket->bind(destination);
      if (!success)
      {
        throw std::runtime_error("Failed to bind socket");
      }
    }

    return socket;
  }
}
