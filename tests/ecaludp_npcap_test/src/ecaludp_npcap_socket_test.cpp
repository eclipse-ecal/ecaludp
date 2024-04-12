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

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>

#include <ecaludp/socket.h>
#include <ecaludp/socket_npcap.h>

#include "atomic_signalable.h"

TEST(EcalUdpNpcapSocket, RAII_unbound)
{
  // Create the socket and destroy it
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});
}

TEST(EcalUdpNpcapSocket, RAII_bound)
{
  // Create the socket, bind and destroy it
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});
  receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
}

TEST(EcalUdpNpcapSocket, RAII_close)
{
  // Create the socket, bind and close it
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});
  receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
  receiver_socket.close();
}

TEST(EcalUdpNpcapSocket, AsyncHelloWorldMessage)
{
  atomic_signalable<int> received_messages(0);

  asio::io_context io_context;

  // Create the sockets
  ecaludp::Socket      sender_socket  (io_context, {'E', 'C', 'A', 'L'});
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});

  // Open the sender_socket
  {
    asio::error_code ec;
    sender_socket.open(asio::ip::udp::v4(), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  // Bind the receiver_socket
  {
    bool success = receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
    ASSERT_EQ(success, true);
  }

  auto work = std::make_unique<asio::io_context::work>(io_context);
  std::thread io_thread([&io_context]() { io_context.run(); });

  std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>("Hello World!");

  // Wait for the next message
  receiver_socket.async_receive_from(*sender_endpoint
                                  , [sender_endpoint, &received_messages, message_to_send](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, ecaludp::Error error)
                                    {
                                      // No error
                                      if (error)
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
  sender_socket.async_send_to({ asio::buffer(*message_to_send) }
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

TEST(EcalUdpSocket, AsyncBigMessage)
{
  constexpr int message_size = 1024 * 1024;
  atomic_signalable<int> received_messages(0);

  asio::io_context io_context;

  // Create the sockets
  ecaludp::Socket       sender_socket  (io_context, {'E', 'C', 'A', 'L'});
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});

  // Open the sender_socket
  {
    asio::error_code ec;
    sender_socket.open(asio::ip::udp::v4(), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  // Bind the receiver_socket
  {
    bool success = receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
    ASSERT_EQ(success, true);
  }

  auto work = std::make_unique<asio::io_context::work>(io_context);
  std::thread io_thread([&io_context]() { io_context.run(); });

  std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>(message_size, 'a');

  // Fill the message with random characters
  std::generate(message_to_send->begin(), message_to_send->end(), []() { return static_cast<char>(std::rand()); });

  // Wait for the next message
  receiver_socket.async_receive_from(*sender_endpoint
                                  , [sender_endpoint, &received_messages, message_to_send](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, ecaludp::Error error)
                                    {
                                      // No error
                                      if (error)
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
  sender_socket.async_send_to({ asio::buffer(*message_to_send) }
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

TEST(ecalupd, AsyncZeroByteMessage)
{
  atomic_signalable<int> received_messages(0);
    
  asio::io_context io_context;
    
  // Create the sockets
  ecaludp::Socket       sender_socket  (io_context, {'E', 'C', 'A', 'L'});
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});
    
  // Open the sender_socket
  {
    asio::error_code ec;
    sender_socket.open(asio::ip::udp::v4(), ec);
    ASSERT_EQ(ec, asio::error_code());
  }
    
  // Bind the receiver_socket
  {
    bool success = receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
    ASSERT_EQ(success, true);
  }
    
  auto work = std::make_unique<asio::io_context::work>(io_context);
  std::thread io_thread([&io_context]() { io_context.run(); });
    
  std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>("");
    
  // Wait for the next message
  receiver_socket.async_receive_from(*sender_endpoint
                                    , [sender_endpoint, &received_messages, message_to_send](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, ecaludp::Error error)
                                      {
                                        // No error
                                        if (error)
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
  sender_socket.async_send_to({ asio::buffer(*message_to_send) }
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

TEST(ecaludp, CancelSyncReceive)
{
  // Create the socket
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});

  // Bind the receiver_socket
  {
    bool success = receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
    ASSERT_EQ(success, true);
  }

  // Create a thread that will blocking-receive the message
  std::thread receiver_thread([&receiver_socket]()
                              {
                                // Wait for the next message
                                std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
                                ecaludp::Error error = ecaludp::Error::GENERIC_ERROR;
                                std::shared_ptr<ecaludp::OwningBuffer> buffer = receiver_socket.receive_from(*sender_endpoint, error);
      
                                // Error must have occured
                                ASSERT_EQ(error, ecaludp::Error::ErrorCode::SOCKET_CLOSED);
                              });

  // Wait 10 ms to make sure the receiver_thread is ready
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Close the socket to un-block the receiver_thread
  receiver_socket.close();

  receiver_thread.join();
}

TEST(ecaludp, SyncHelloWorldMessage)
{
  atomic_signalable<int> received_messages(0);

  asio::io_context io_context;

  // Create the sockets
  ecaludp::Socket      sender_socket  (io_context, {'E', 'C', 'A', 'L'});
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});

  // Open the sender_socket
  {
    asio::error_code ec;
    sender_socket.open(asio::ip::udp::v4(), ec);
    ASSERT_EQ(ec, asio::error_code());
  }

  // Bind the receiver_socket
  {
    bool success = receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
    ASSERT_EQ(success, true);
  }

  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>("Hello World!");

  // Create a thread that will blocking-receive the message
  std::thread receiver_thread([&receiver_socket, message_to_send, &received_messages]()
                              {
                                // Wait for the next message
                                std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
                                ecaludp::Error error = ecaludp::Error::GENERIC_ERROR;
                                std::shared_ptr<ecaludp::OwningBuffer> buffer = receiver_socket.receive_from(*sender_endpoint, error);
      
                                // No error
                                ASSERT_FALSE(bool(error));
                                    
                                // compare the messages
                                std::string received_string(static_cast<const char*>(buffer->data()), buffer->size());
                                ASSERT_EQ(received_string, *message_to_send);

                                received_messages++;
                              });

  // Wait 10 ms to make sure the receiver_thread is ready
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Send a message
  {
    asio::error_code ec;
    sender_socket.send_to({ asio::buffer(*message_to_send) }
                        , asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000)
                        , 0
                        , ec);
  }

  // Wait up to 1s for the message to be received
  received_messages.wait_for([](int received_messages) { return received_messages == 1; }, std::chrono::milliseconds(1000));

  ASSERT_EQ(received_messages, 1);

  // Close the socket to un-block and fail this test, if the receiver_thread should still be waiting
  receiver_socket.close();

  // Wait for the receiver_thread to finish
  receiver_thread.join();
}

TEST(ecaludp, SyncBigMessage)
{
  constexpr int message_size = 1024 * 1024;

  atomic_signalable<int> received_messages(0);
    
  asio::io_context io_context;
    
  // Create the sockets
  ecaludp::Socket       sender_socket  (io_context, {'E', 'C', 'A', 'L'});
  ecaludp::SocketNpcap receiver_socket({'E', 'C', 'A', 'L'});
    
  // Open the sender_socket
  {
    asio::error_code ec;
    sender_socket.open(asio::ip::udp::v4(), ec);
    ASSERT_FALSE(ec);
  }

  // Increase the outgoing buffer size
  {
    asio::error_code ec;
    sender_socket.set_option(asio::socket_base::send_buffer_size(1024 * 1024 * 5), ec);
    ASSERT_FALSE(ec);
  }
    
  // Bind the receiver_socket
  {
    bool success = receiver_socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
    ASSERT_TRUE(success);
  }
    
  std::shared_ptr<std::string> message_to_send = std::make_shared<std::string>(message_size, 'a');
    
  // Fill the message with random characters
  std::generate(message_to_send->begin(), message_to_send->end(), []() { return static_cast<char>(std::rand()); });
    
  // Create a thread that will blocking-receive the message
  std::thread receiver_thread([&receiver_socket, message_to_send, &received_messages]()
                              {
                                // Wait for the next message
                                std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
                                ecaludp::Error error = ecaludp::Error::GENERIC_ERROR;
                                std::shared_ptr<ecaludp::OwningBuffer> buffer = receiver_socket.receive_from(*sender_endpoint, error);
        
                                // No error
                                ASSERT_FALSE(bool(error));
                                        
                                // compare the messages
                                std::string received_string(static_cast<const char*>(buffer->data()), buffer->size());
                                ASSERT_EQ(received_string, *message_to_send);

                                received_messages++;
                              });
    
  // Wait 10 ms to make sure the receiver_thread is ready
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
  // Send a message
  {
      asio::error_code ec;
      sender_socket.send_to({ asio::buffer(*message_to_send) }
                          , asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000)
                          , 0
                          , ec);
  }

  // Wait up to 1s for the message to be received
  received_messages.wait_for([](int received_messages) { return received_messages == 1; }, std::chrono::milliseconds(1000));

  ASSERT_EQ(received_messages, 1);

  // Close the socket to un-block and fail this test, if the receiver_thread should still be waiting
  receiver_socket.close();

  // Wait for the receiver_thread to finish
  receiver_thread.join();
}

