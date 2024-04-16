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

#include "sender_async.h"

#include <cstddef>
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

SenderAsync::SenderAsync(const SenderParameters& parameters)
  : Sender(parameters)
{
  std::cout << "Sender implementation: Asynchronous asio" << std::endl;
}

SenderAsync::~SenderAsync()
{
  if (socket_)
  {
    socket_->cancel();
  }

  io_context_thread_->join();
}

void SenderAsync::start() 
{
  try
  {
     socket_ = SocketBuilderAsio::CreateSendSocket(io_context_, parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what() << std::endl;
    return; // TODO: Exit the app?
  }

  auto message = std::make_shared<std::string>(parameters_.message_size, 'a');
  auto endpoint = asio::ip::udp::endpoint(asio::ip::make_address(parameters_.ip), parameters_.port);

  send_message(message, endpoint);

  io_context_thread_ = std::make_unique<std::thread>([this](){ io_context_.run(); });
}

void SenderAsync::send_message(const std::shared_ptr<const std::string>& message, const asio::ip::udp::endpoint& endpoint)
{

  socket_->async_send_to( asio::buffer(*message)
                        , endpoint
                        , [this, message, endpoint](asio::error_code ec)
                          {
                            if (ec)
                            {
                              std::cout << "Error sending: " << ec.message() << std::endl;
                              socket_->close();
                              return;
                            }

                            {
                              std::unique_lock<std::mutex> lock(statistics_mutex_);

                              //bytes_raw_     += bytes_sent; // TODO: the current implementation doesn't return the raw number of bytes sent
                              bytes_payload_ += message->size();
                              messages_sent_ ++;
                            }

                            this->send_message(message, endpoint);
                          });

}
