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

#include "sender_sync.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <asio.hpp>

#include "ecaludp/socket.h"
#include "sender.h"
#include "sender_parameters.h"
#include "socket_builder_asio.h"

SenderSync::SenderSync(const SenderParameters& parameters)
  : Sender(parameters)
{
  std::cout << "Sender implementation: Synchronous asio\n";
}

SenderSync::~SenderSync()
{
  if (send_thread_ && send_thread_->joinable())
  {
    send_thread_->join();
  }
}

void SenderSync::start()
{
  send_thread_ = std::make_unique<std::thread>(&SenderSync::send_loop, this);
}

void SenderSync::send_loop()
{
  asio::io_context io_context;

  std::shared_ptr<ecaludp::Socket> send_socket;
  try
  {
     send_socket = SocketBuilderAsio::CreateSendSocket(io_context, parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what() << '\n';
    std::exit(1);
  }

  const std::string message = std::string(parameters_.message_size, 'a');
  const asio::ip::udp::endpoint destination(asio::ip::address::from_string(parameters_.ip), parameters_.port);

  while (true)
  {
    {
      asio::error_code ec;
      auto bytes_sent = send_socket->send_to(asio::buffer(message), destination, 0, ec);

      if (ec)
      {
        std::cerr << "Error sending message: " << ec.message() << '\n';
        break;
      }

      {
        const std::lock_guard<std::mutex> lock(statistics_mutex_);
      
        if (is_stopped_)
          break;

        bytes_raw_     += bytes_sent;
        bytes_payload_ += message.size();
        messages_sent_ ++;
      }
    }
  }

  {
    asio::error_code ec;
    send_socket->shutdown(asio::socket_base::shutdown_both, ec); // NOLINT(bugprone-unused-return-value) The function also returns the error_code, but we already got it via the parameter
    if (ec)
    {
      std::cerr << "Error shutting down socket: " << ec.message() << '\n';
    }
  }

  {
    asio::error_code ec;
    send_socket->close(ec); // NOLINT(bugprone-unused-return-value) The function also returns the error_code, but we already got it via the parameter
    if (ec)
    {
      std::cerr << "Error closing socket: " << ec.message() << '\n';
    }
  }
}
