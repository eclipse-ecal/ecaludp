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

#include <iostream>
#include <string>

SenderSync::SenderSync(size_t message_size, size_t max_udp_datagram_size, int buffer_size)
  : Sender(message_size, max_udp_datagram_size, buffer_size)
{
  std::cout << "Sender implementation: Synchronous asio" << std::endl;
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

  ecaludp::Socket         send_socket(io_context, {'E', 'C', 'A', 'L'});
  asio::ip::udp::endpoint destination(asio::ip::address_v4::loopback(), 14000);

  send_socket.set_max_udp_datagram_size(max_udp_datagram_size_);

  {
    asio::error_code ec;
    send_socket.open(destination.protocol(), ec);
    if (ec)
    {
      std::cerr << "Error opening socket: " << ec.message() << std::endl;
      return;
    }
  }

  // Set sent buffer size
  if (buffer_size_ > 0)
  {
    asio::socket_base::send_buffer_size option(buffer_size_);

    asio::error_code ec;
    send_socket.set_option(option, ec);
    if (ec)
    {
      std::cerr << "Error setting send buffer size: " << ec.message() << std::endl;
    }
  }

  std::string message = std::string(message_size_, 'a');

  while (true)
  {
    {

      asio::error_code ec;
      auto bytes_sent = send_socket.send_to(asio::buffer(message), destination, 0, ec);

      if (ec)
      {
        std::cerr << "Error sending message: " << ec.message() << std::endl;
        break;
      }

      {
        std::unique_lock<std::mutex> lock(statistics_mutex_);
      
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
    send_socket.shutdown(asio::socket_base::shutdown_both, ec);
    if (ec)
    {
      std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
    }
  }

  {
    asio::error_code ec;
    send_socket.close(ec);
    if (ec)
    {
      std::cerr << "Error closing socket: " << ec.message() << std::endl;
    }
  }
}
