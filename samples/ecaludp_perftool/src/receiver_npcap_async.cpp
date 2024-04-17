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

#include "receiver_npcap_async.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <asio.hpp>

#include "ecaludp/socket.h"
#include "receiver.h"
#include "socket_builder_npcap.h"

ReceiverNpcapAsync::ReceiverNpcapAsync(const ReceiverParameters& parameters)
  : Receiver(parameters)
{
  std::cout << "Receiver implementation: Asynchronous NPCAP" << std::endl;
}

ReceiverNpcapAsync::~ReceiverNpcapAsync()
{
  if (socket_)
  {
    socket_->close();
  }
}

void ReceiverNpcapAsync::start()
{
  try
  {
     socket_ = SocketBuilderNpcap::CreateReceiveSocket(parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what() << std::endl;
    return; // TODO: Exit the app?
  }

  receive_message();
}

void ReceiverNpcapAsync::receive_message()
{
  asio::ip::udp::endpoint endpoint;

  socket_->async_receive_from(endpoint,
                              [this](const std::shared_ptr<ecaludp::OwningBuffer>& message, ecaludp::Error& error)
                              {
                                if (error)
                                {
                                  std::cout << "Error sending: " << error.ToString() << std::endl;
                                  socket_->close();
                                  return;
                                }

                                {
                                  std::unique_lock<std::mutex> lock(statistics_mutex_);

                                  bytes_payload_     += message->size();
                                  messages_received_ ++;
                                }

                                receive_message();
                              });
}
