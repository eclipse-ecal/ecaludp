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

#include <iostream>

#include <asio.hpp>
#include <ecaludp/socket.h>

#include <thread>

int main()
{
  asio::io_context io_context;

  // Create a socket
  ecaludp::Socket socket(io_context, {'E', 'C', 'A', 'L'});

  // Open the socket
  {
    asio::error_code ec;
    socket.open(asio::ip::udp::v4(), ec);
  }

  // Bind the socket
  {
    asio::error_code ec;
    socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000), ec);
  }

  auto work = std::make_unique<asio::io_context::work>(io_context);
  std::thread io_thread([&io_context]() { io_context.run(); });

  std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>("Hello World!");

  // Wait for the next message
  socket.async_receive_from(*sender_endpoint
                                , [sender_endpoint, message_to_send](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, asio::error_code ec)
                                  {
                                  });

  // Send a message
  socket.async_send_to({ asio::buffer(*message_to_send) }
                      , asio::ip::udp::endpoint(asio::ip::address_v4::loopback()
                      , 14000)
                      , [message_to_send](asio::error_code ec)
                        {
                        });

  work.reset();
  io_thread.join();
}
