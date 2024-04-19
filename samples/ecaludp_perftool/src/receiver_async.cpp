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

#include "receiver_async.h"

#include <cstdlib>
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

ReceiverAsync::ReceiverAsync(const ReceiverParameters& parameters)
  : Receiver(parameters)
{
  std::cout << "Receiver implementation: Asynchronous asio\n";
}

ReceiverAsync::~ReceiverAsync()
{
  if (socket_)
  {
    asio::error_code ec;
    socket_->cancel(ec); // NOLINT(bugprone-unused-return-value) The function also returns the error_code, but we already got it via the parameter

    if (ec)
      std::cerr << "Error cancelling socket: " << ec.message() << '\n';
  }

  if(work_)
    work_.reset();

  if (io_context_thread_->joinable())
    io_context_thread_->join();
}

void ReceiverAsync::start()
{
  try
  {
     socket_ = SocketBuilderAsio::CreateReceiveSocket(io_context_, parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what() << '\n';
    std::exit(1);
  }

  auto endpoint = asio::ip::udp::endpoint(asio::ip::make_address(parameters_.ip), parameters_.port);

  receive_message();

  work_ = std::make_unique<asio::io_context::work>(io_context_);

  io_context_thread_ = std::make_unique<std::thread>([this](){ io_context_.run(); });
}

void ReceiverAsync::receive_message()
{
  auto endpoint = std::make_shared<asio::ip::udp::endpoint>();

  socket_->async_receive_from(*endpoint,
                              [this, endpoint](const std::shared_ptr<ecaludp::OwningBuffer>& message, const asio::error_code& ec)
                              {
                                if (ec)
                                {
                                  std::cerr << "Error sending: " << ec.message() << '\n';
                                  socket_->close();
                                  return;
                                }

                                {
                                  const std::lock_guard<std::mutex> lock(statistics_mutex_);

                                  bytes_payload_     += message->size();
                                  messages_received_ ++;
                                }

                                receive_message();
                              });
}
