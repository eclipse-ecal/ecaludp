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

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <asio.hpp> // IWYU pragma: keep

#include <ecaludp/socket.h>


std::shared_ptr<asio::io_context> io_context_;

std::shared_ptr<ecaludp::Socket>    socket_;
std::shared_ptr<asio::steady_timer> send_timer_;

void send_package()
{
  auto douglas_adams_buffer_1 = std::make_shared<std::string>("In the beginning the Universe was created.");
  auto douglas_adams_buffer_2 = std::make_shared<std::string>(" ");
  auto douglas_adams_buffer_3 = std::make_shared<std::string>("This had made many people very angry and has been widely regarded as a bad move.");

  socket_->async_send_to({ asio::buffer(*douglas_adams_buffer_1), asio::buffer(*douglas_adams_buffer_2), asio::buffer(*douglas_adams_buffer_3) }
                        , asio::ip::udp::endpoint(asio::ip::address_v4::loopback()
                        , 14000)
                        , [douglas_adams_buffer_1, douglas_adams_buffer_2, douglas_adams_buffer_3](asio::error_code ec)
                          {
                            if (ec)
                            {
                              std::cout << "Error sending: " << ec.message() << std::endl;
                              return;
                            }

                            send_timer_->expires_from_now(std::chrono::milliseconds(500));
                            send_timer_->async_wait([](asio::error_code ec)
                                                    {
                                                      if (ec)
                                                      {
                                                        std::cout << "Error waiting: " << ec.message() << std::endl;
                                                        return;
                                                      }

                                                      send_package();
                                                    });
                          });
}

void receive_package()
{
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();

  socket_->async_receive_from(*sender_endpoint
                              , [sender_endpoint](const std::shared_ptr<ecaludp::OwningBuffer>& buffer, asio::error_code ec)
                                {
                                  if (ec)
                                  {
                                    std::cout << "Error receiving: " << ec.message() << std::endl;
                                    return;
                                  }

                                  std::string received_string(static_cast<const char*>(buffer->data()), buffer->size());
                                  std::cout << "Received " << buffer->size() << " bytes from " << sender_endpoint->address().to_string() << ":" << sender_endpoint->port() << ": " << received_string << std::endl;


                                  receive_package();
                                });
}

int main(int argc, char** argv)
{
  std::cout << "Starting...\n";

  io_context_ = std::make_shared<asio::io_context>();

  socket_ = std::make_shared<ecaludp::Socket>(*io_context_, std::array<char, 4>{'E', 'C', 'A', 'L'});

  {
    asio::error_code ec;
    socket_->open(asio::ip::udp::v4(), ec);

    if (ec)
    {
      std::cout << "Error opening socket: " << ec.message() << std::endl;
      return -1;
    }
  }

  {
    asio::error_code ec;
    socket_->bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000), ec);

    if (ec)
    {
      std::cout << "Error binding socket: " << ec.message() << std::endl;
      return -1;
    }
  }

  send_timer_ = std::make_shared<asio::steady_timer>(*io_context_);

  asio::io_context::work work(*io_context_);

  receive_package();
  send_package();

  io_context_->run();

  return 0;
}
