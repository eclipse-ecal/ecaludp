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

#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <asio.hpp>

#include "ecaludp/socket.h"
#include "receiver.h"
#include "receiver_parameters.h"
#include "socket_builder_asio.h"

ReceiverSync::ReceiverSync(const ReceiverParameters& parameters)
  : Receiver(parameters)
{
  std::cout << "Receiver implementation: Synchronous asio\n";
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

  std::shared_ptr<ecaludp::Socket> receive_socket;
  try
  {
     receive_socket = SocketBuilderAsio::CreateReceiveSocket(io_context, parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what() << '\n';
    exit(1);
  }

  asio::ip::udp::endpoint destination(asio::ip::address::from_string(parameters_.ip), parameters_.port);

  while (true)
  {
    {

      asio::error_code ec;
      auto payload_buffer = receive_socket->receive_from(destination, 0, ec);

      if (ec)
      {
        std::cerr << "Error receiving message: " << ec.message() << '\n';
        break;
      }

      {
        const std::lock_guard<std::mutex> lock(statistics_mutex_);
      
        if (is_stopped_)
          break;

        bytes_payload_ += payload_buffer->size();
        messages_received_ ++;
      }
    }
  }

  {
    asio::error_code ec;
    receive_socket->shutdown(asio::socket_base::shutdown_both, ec); // NOLINT(bugprone-unused-return-value) The function also returns the error_code, but we already got it via the parameter
    if (ec)
    {
      std::cerr << "Error shutting down socket: " << ec.message() << '\n';
    }
  }

  {
    asio::error_code ec;
    receive_socket->close(ec); // NOLINT(bugprone-unused-return-value) The function also returns the error_code, but we already got it via the parameter
    if (ec)
    {
      std::cerr << "Error closing socket: " << ec.message() << '\n';
    }
  }
}
