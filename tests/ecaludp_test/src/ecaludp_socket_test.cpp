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

#include <gtest/gtest.h>

#include <asio.hpp> // IWYU pragma: keep

#include <ecaludp/socket.h>

#include "atomic_signalable.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

TEST(EcalUdpSocket, HelloWorldMessage)
{
  atomic_signalable<int> received_messages(0);

  asio::io_context io_context;

  // Create a socket
  ecaludp::Socket socket(io_context, {'E', 'C', 'A', 'L'});

  // Open the socket
  {
    asio::error_code ec;
    socket.open(asio::ip::udp::v4(), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  // Bind the socket
  {
    asio::error_code ec;
    socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  auto work = std::make_unique<asio::io_context::work>(io_context);
  std::thread io_thread([&io_context]() { io_context.run(); });

  std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>("Hello World!");

  // Wait for the next message
  socket.async_receive_from(*sender_endpoint
                                , [sender_endpoint, &received_messages, message_to_send](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, asio::error_code ec)
                                  {
                                    // No error
                                    if (ec)
                                    {
                                      FAIL();
                                    }
                                    
                                    // compare the messages
                                    std::string received_string(static_cast<const char*>(buffer->data()), buffer->size());
                                    ASSERT_EQ(received_string, *message_to_send);

                                    // increment
                                    received_messages++;
                                  });

  // Send a message
  socket.async_send_to({ asio::buffer(*message_to_send) }
                      , asio::ip::udp::endpoint(asio::ip::address_v4::loopback()
                      , 14000)
                      , [message_to_send](asio::error_code ec)
                        {
                          // No error
                          ASSERT_EQ(ec, asio::error_code());
                        });

  // Wait for the message to be received
  received_messages.wait_for([](int received_messages) { return received_messages == 1; }, std::chrono::milliseconds(100));

  ASSERT_EQ(received_messages, 1);

  work.reset();
  io_thread.join();
}

TEST(EcalUdpSocket, BigMessage)
{
  atomic_signalable<int> received_messages(0);

  asio::io_context io_context;

  // Create a socket
  ecaludp::Socket socket(io_context, {'E', 'C', 'A', 'L'});

  // Open the socket
  {
    asio::error_code ec;
    socket.open(asio::ip::udp::v4(), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  // Bind the socket
  {
    asio::error_code ec;
    socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  auto work = std::make_unique<asio::io_context::work>(io_context);
  std::thread io_thread([&io_context]() { io_context.run(); });

  std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>(1024 * 1024, 'a');

  // Fill the message with random characters
  std::generate(message_to_send->begin(), message_to_send->end(), []() { return static_cast<char>(std::rand()); });

  // Wait for the next message
  socket.async_receive_from(*sender_endpoint
                                , [sender_endpoint, &received_messages, message_to_send](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, asio::error_code ec)
                                  {
                                    // No error
                                    if (ec)
                                    {
                                      FAIL();
                                    }
                                    
                                    // compare the messages
                                    std::string received_string(static_cast<const char*>(buffer->data()), buffer->size());
                                    ASSERT_EQ(received_string, *message_to_send);

                                    // increment
                                    received_messages++;
                                  });

  // Send a message
  socket.async_send_to({ asio::buffer(*message_to_send) }
                      , asio::ip::udp::endpoint(asio::ip::address_v4::loopback()
                      , 14000)
                      , [message_to_send](asio::error_code ec)
                        {
                          // No error
                          ASSERT_EQ(ec, asio::error_code());
                        });

  // Wait for the message to be received
  received_messages.wait_for([](int received_messages) { return received_messages == 1; }, std::chrono::milliseconds(1000));

  ASSERT_EQ(received_messages, 1);

  work.reset();
  io_thread.join();
}