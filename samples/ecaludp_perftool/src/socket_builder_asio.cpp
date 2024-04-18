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

#include "socket_builder_asio.h"
#include "ecaludp/socket.h"
#include "receiver_parameters.h"
#include "sender_parameters.h"

#include <array>
#include <memory>

#include <asio.hpp>
#include <stdexcept>

namespace SocketBuilderAsio
{
  std::shared_ptr<ecaludp::Socket> CreateSendSocket(asio::io_context& io_context, const SenderParameters& parameters)
  {
    auto socket = std::make_shared<ecaludp::Socket>(io_context, std::array<char, 4>{'E', 'C', 'A', 'L'});
    
    asio::ip::address ip_address {};
    {
      asio::error_code ec;
      ip_address = asio::ip::make_address(parameters.ip, ec);
      if (ec)
      {
        throw std::runtime_error("Invalid IP address: " + parameters.ip);
      }
    }

    asio::ip::udp::endpoint destination(ip_address, parameters.port);

    if (parameters.max_udp_datagram_size > 0)
    {
      socket->set_max_udp_datagram_size(parameters.max_udp_datagram_size);
    }

    {
      asio::error_code ec;
      socket->open(destination.protocol(), ec);
      if (ec)
      {
        throw std::runtime_error("Failed to open socket: " + ec.message());
      }
    }

    // Set sent buffer size
    if (parameters.buffer_size > 0)
    {
      asio::socket_base::send_buffer_size option(parameters.buffer_size);

      asio::error_code ec;
      socket->set_option(option, ec);
      if (ec)
      {
        throw std::runtime_error("Failed to set send buffer size: " + ec.message());
      }
    }

    return socket;
  }

  std::shared_ptr<ecaludp::Socket> CreateReceiveSocket(asio::io_context& io_context, const ReceiverParameters& parameters)
  {
    auto socket = std::make_shared<ecaludp::Socket>(io_context, std::array<char, 4>{'E', 'C', 'A', 'L'});
    
    asio::ip::address ip_address {};
    {
      asio::error_code ec;
      ip_address = asio::ip::make_address(parameters.ip, ec);
      if (ec)
      {
        throw std::runtime_error("Invalid IP address: " + parameters.ip);
      }
    }

    asio::ip::udp::endpoint destination(ip_address, parameters.port);

    {
      asio::error_code ec;
      socket->open(destination.protocol(), ec);
      if (ec)
      {
        throw std::runtime_error("Failed to open socket: " + ec.message());
      }
    }

    {
      asio::error_code ec;
      socket->bind(destination, ec);
      if (ec)
      {
        throw std::runtime_error("Failed to bind socket: " + ec.message());
      }
    }

    // Set receive buffer size
    if (parameters.buffer_size > 0)
    {
      asio::socket_base::receive_buffer_size option(parameters.buffer_size);

      asio::error_code ec;
      socket->set_option(option, ec);
      if (ec)
      {
        throw std::runtime_error("Failed to set receive buffer size: " + ec.message());
      }
    }

    // Set reuse address
    {
      asio::socket_base::reuse_address option(true);

      asio::error_code ec;
      socket->set_option(option, ec);
      if (ec)
      {
        throw std::runtime_error("Failed to set reuse address: " + ec.message());
      }
    }

    return socket;
  }
}
