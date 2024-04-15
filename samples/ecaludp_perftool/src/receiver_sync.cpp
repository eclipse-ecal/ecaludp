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

#include "receiver_sync.h"
#include "ecaludp/socket.h"
#include "receiver.h"

#include <iostream>
#include <memory>
#include <mutex>

#include <asio.hpp>
#include <thread>

ReceiverSync::ReceiverSync(int buffer_size)
  : Receiver(buffer_size)
{
  std::cout << "Receiver implementation: Synchronous asio" << std::endl;
}

ReceiverSync::~ReceiverSync()
{
  if (receive_thread_ && receive_thread_->joinable())
  {
    receive_thread_->join();
  }
}

void ReceiverSync::start()
{
  receive_thread_ = std::make_unique<std::thread>(&ReceiverSync::receive_loop, this);
}

void ReceiverSync::receive_loop()
{
  asio::io_context io_context;

  ecaludp::Socket         receive_socket(io_context, {'E', 'C', 'A', 'L'});
  asio::ip::udp::endpoint destination(asio::ip::address_v4::loopback(), 14000);

  {
    asio::error_code ec;
    receive_socket.open(destination.protocol(), ec);
    if (ec)
    {
      std::cerr << "Error opening socket: " << ec.message() << std::endl;
      return;
    }
  }

  {
    asio::error_code ec;
    receive_socket.bind(destination, ec);
    if (ec)
    {
      std::cerr << "Error binding socket: " << ec.message() << std::endl;
      return;
    }
  }

  if (buffer_size_ > 0)
  {
    // Set receive buffer size
    asio::socket_base::receive_buffer_size option(buffer_size_);
    asio::error_code ec;
    receive_socket.set_option(option, ec);
    if (ec)
    {
      std::cerr << "Error setting receive buffer size: " << ec.message() << std::endl;
      return;
    }
  }

  while (true)
  {
    {

      asio::error_code ec;
      auto payload_buffer = receive_socket.receive_from(destination, 0, ec);

      if (ec)
      {
        std::cerr << "Error receiving message: " << ec.message() << std::endl;
        break;
      }

      {
        std::unique_lock<std::mutex> lock(statistics_mutex_);
      
        if (is_stopped_)
          break;

        bytes_payload_ += payload_buffer->size();
        messages_received_ ++;
      }
    }
  }

  {
    asio::error_code ec;
    receive_socket.shutdown(asio::socket_base::shutdown_both, ec);
    if (ec)
    {
      std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
    }
  }

  {
    asio::error_code ec;
    receive_socket.close(ec);
    if (ec)
    {
      std::cerr << "Error closing socket: " << ec.message() << std::endl;
    }
  }
}
